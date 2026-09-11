#include "queue.h"
#include "heap.h"
#include "task.h"
#include "port.h"


static inline uint32_t Remaining_Timeout(uint32_t original_timeout, uint32_t start_tick)
{
    if (original_timeout == PORT_MAX_TIMEOUT)
        return PORT_MAX_TIMEOUT;

    uint32_t elapsed = Task_Get_Tick_Count() - start_tick;
    return (elapsed >= original_timeout) ? 0 : (original_timeout - elapsed);
}

static inline void Copy_Object(void* dest, void* src, size_t object_size)
{
    uint8_t* d = (uint8_t*) dest;
    uint8_t* s = (uint8_t*) src;
    for (size_t i = 0; i < object_size; i++)
        d[i] = s[i];
}


int Queue_Create(Queue_t* handle, size_t queue_size, size_t object_size)
{
    if (!queue_size || !object_size)
        return 1;
    Queue_t created_queue = Heap_Alloc(sizeof(Queue));
    if (!created_queue)
        return 2;
    int status_sem_spa,status_sem_ita, status_sem_mtx; 
    status_sem_spa = Semaphore_Create(&created_queue->space_available, queue_size, SEMAPHORE_FIFO);
    status_sem_ita = Semaphore_Create(&created_queue->item_available, 0, SEMAPHORE_FIFO);
    status_sem_mtx = Semaphore_Create(&created_queue->mutex, 1, SEMAPHORE_FIFO);

    if (status_sem_spa || status_sem_ita || status_sem_mtx)
    {
        if (!status_sem_spa) Semaphore_Delete(created_queue->space_available);
        if (!status_sem_ita) Semaphore_Delete(created_queue->item_available);
        if (!status_sem_mtx) Semaphore_Delete(created_queue->mutex);
        Heap_Free(created_queue);
        return 3;
    }

    void* obj_mem = Heap_Alloc(queue_size * object_size);
    if (!obj_mem)
    {
        Semaphore_Delete(created_queue->space_available);
        Semaphore_Delete(created_queue->item_available);
        Semaphore_Delete(created_queue->mutex);
        Heap_Free(created_queue);
        return 4;
    }

    created_queue->object_size = object_size;
    created_queue->queue_size = queue_size;
    created_queue->head = 0;
    created_queue->tail = 0;
    created_queue->objects = obj_mem;
    *handle = created_queue;
    return 0;
}

int Queue_Put(Queue_t queue, void* buffer, uint32_t timeout)
{
    if (!queue || !buffer)
        return (int) SEM_ERROR;

    uint32_t start = Task_Get_Tick_Count();
    SEM_STATUS status = Semaphore_Acquire(queue->space_available, timeout);
    if (status != SEM_ACQUIRED)
        return (int) status;

    uint32_t remaining = Remaining_Timeout(timeout, start);
    status = Semaphore_Acquire(queue->mutex, remaining);
    if (status != SEM_ACQUIRED)
    {
        Semaphore_Release(queue->space_available);
        return (int) status;
    }

    void* dest = (uint8_t*) queue->objects + (queue->tail * queue->object_size);
    Copy_Object(dest, buffer, queue->object_size);
    queue->tail = (queue->tail + 1) % queue->queue_size;

    Semaphore_Release(queue->mutex);
    Semaphore_Release(queue->item_available);

    return 0;
}

int Queue_Get(Queue_t queue, void* buffer, uint32_t timeout)
{
    if (!queue || !buffer)
        return (int) SEM_ERROR;

    uint32_t start = Task_Get_Tick_Count();
    SEM_STATUS status = Semaphore_Acquire(queue->item_available, timeout);
    if (status != SEM_ACQUIRED)
        return (int) status;

    uint32_t remaining = Remaining_Timeout(timeout, start);
    status = Semaphore_Acquire(queue->mutex, remaining);
    if (status != SEM_ACQUIRED)
    {
        Semaphore_Release(queue->item_available);
        return (int) status;
    }

    void* src = (uint8_t*) queue->objects + (queue->head * queue->object_size);
    Copy_Object(buffer, src, queue->object_size);
    queue->head = (queue->head + 1) % queue->queue_size;

    Semaphore_Release(queue->mutex);
    Semaphore_Release(queue->space_available);

    return 0;
}

int Queue_Empty(Queue_t queue)
{
    if (!queue) return 1;
    return !Semaphore_Get_Value(queue->item_available);
}

int Queue_Delete(Queue_t queue)
{
    if (!queue) return 1;

    Semaphore_Delete(queue->space_available);
    Semaphore_Delete(queue->item_available);
    Semaphore_Delete(queue->mutex);

    Heap_Free(queue->objects);
    Heap_Free(queue);

    return 0;
}


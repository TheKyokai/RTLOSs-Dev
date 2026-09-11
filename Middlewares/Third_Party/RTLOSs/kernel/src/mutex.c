#include "mutex.h"
#include "port.h"
#include "scheduler.h"
#include "heap.h"

static List global_mutex_timeout_list;

void MTX_Init()
{
    List_Init(&global_mutex_timeout_list);
}

static inline void MTX_Timeout_Insert(TCB* tcb, uint32_t timeout);


static inline int MTX_Priority_Comparison(void* current, void* other)
{
    return (int)((TCB*)current)->priority - (int)((TCB*)other)->priority;
}

int MTX_Create(Mutex_t *handle, uint32_t initial_val, int priority)
{
    if (!handle)    return 1;
    if (priority != MUTEX_FIFO && priority != MUTEX_PRIORITY)   return 3;

    Mutex* created_mutex = (Mutex*) HEAP_Alloc(sizeof(Mutex));

    if (!created_mutex)
        return 2;

    created_mutex->val = initial_val;
    List_Init(&created_mutex->mutex_queue);
    created_mutex->holder = NULL;
    created_mutex->priority = priority;

    *handle = created_mutex;

    return 0;
}

SEM_STATUS MTX_Acquire(Mutex_t mutex, uint32_t timeout)
{
    if (!mutex)
        return SEM_ERROR;

    if (timeout == 0)
    {
        if (mutex->val == 0)
            return SEM_TIMEOUT;
        mutex->val--;
        mutex->holder = TCB_Current;
        TCB_Current->owned_mutex_cnt++;
        return SEM_ACQUIRED;
    }

    if (mutex->val == 0)
    {
        if (timeout < PORT_MAX_TIMEOUT)
        {
            TCB_Current->timed_wait = 1;
            MTX_Timeout_Insert(TCB_Current, timeout);
        }
        TCB_Current->status = TASK_BLOCKED;
        TCB_Current->waited_mutex = mutex;
        if (mutex->priority)
            List_Insert_Sorted(&mutex->mutex_queue, &TCB_Current->sem_node, MTX_Priority_Comparison);
        else
            List_Insert_Back(&mutex->mutex_queue, &TCB_Current->sem_node);
        Port_Yield();   
    }
    else
    {
        --mutex->val;
        mutex->holder = TCB_Current;
        TCB_Current->sem_status = SEM_ACQUIRED;
        TCB_Current->owned_mutex_cnt++;
    }

    return TCB_Current->sem_status;
}

void MTX_Release(Mutex_t mutex)
{
    if (!mutex) return;

    if (TCB_Current != mutex->holder)
        return;

    mutex->holder->owned_mutex_cnt--;
    
    if (!mutex->mutex_queue.head)
    {
        mutex->val++;   
        mutex->holder = NULL;
        return;
    }

    TCB* ready_tcb = (TCB*) List_Remove_Front(&mutex->mutex_queue);
    ready_tcb->sem_status = SEM_ACQUIRED;
    ready_tcb->waited_mutex = NULL;
    mutex->holder = ready_tcb;
    ready_tcb->owned_mutex_cnt++;


    if (ready_tcb->timed_wait)
    {
        ready_tcb->timed_wait = 0;
        // Adjust time delta for next waiting tcb
        if (ready_tcb->list_node.next)
            ((TCB*) ready_tcb->list_node.next->data)->timeout += ready_tcb->timeout;
        ready_tcb->timeout = 0;
        List_Remove(&global_mutex_timeout_list, &ready_tcb->list_node);
    }

    if (ready_tcb->status == TASK_BLOCKED)
        ready_tcb->status = TASK_READY;
    Scheduler_Put(ready_tcb);
}

int MTX_Delete(Mutex_t mutex)
{
    if (!mutex) return 1;

    while (mutex->mutex_queue.head)
    {
        TCB* tcb = (TCB*) List_Remove_Front(&mutex->mutex_queue);

        if (tcb->timed_wait)
        {
            tcb->timed_wait = 0;
            // Adjust time delta for next waiting tcb
            if (tcb->list_node.next)
                ((TCB*) tcb->list_node.next->data)->timeout += tcb->timeout;
            tcb->timeout = 0;
            List_Remove(&global_mutex_timeout_list, &tcb->list_node);
        }

        tcb->sem_status = SEM_DEAD;
        tcb->waited_mutex = NULL;
        if (tcb->status == TASK_BLOCKED)
            tcb->status = TASK_READY;
        Scheduler_Put(tcb);
    }

    HEAP_Free(mutex);
    return 0;
}


// API

int Mutex_Create(Mutex_t *handle, uint32_t initial_val, int priority)
{
    struct MTX_Create_args args = { handle, initial_val, priority };
    return (int) Port_Syscall(SVC_MUTEX_CREATE, &args);
}

SEM_STATUS Mutex_Acquire(Mutex_t mutex, uint32_t timeout)
{
    struct MTX_Acquire_args args = { mutex, timeout };
    SEM_STATUS ret_stat = (SEM_STATUS) Port_Syscall(SVC_MUTEX_ACQUIRE, &args);
    if (timeout != 0)
    {
        ret_stat = Task_Current_Task->sem_status;
        Task_Current_Task->sem_status = SEM_NONE;
    }
    return ret_stat;
}

void Mutex_Release(Mutex_t mutex)
{
    struct MTX_Release_args args = { mutex };
    Port_Syscall(SVC_MUTEX_RELEASE, &args);
}

int Mutex_Delete(Mutex_t mutex)
{
    struct MTX_Delete_args args = { mutex };
    return (int) Port_Syscall(SVC_MUTEX_DELETE, &args);
}

// Called from TCB_SysTick_Tick -> other interrupt using RTLOSs API must be masked
void MTX_Tick_Update()
{
    if (!global_mutex_timeout_list.head)  return;

    TCB* tcb = (TCB*) List_Peek_Front(&global_mutex_timeout_list);
    --tcb->timeout;
    while (tcb && tcb->timeout == 0)
    {
        // Remove from timeout list
        List_Remove_Front(&global_mutex_timeout_list);
        // Remove from mutex queue
        List_Remove(&tcb->waited_mutex->mutex_queue, &tcb->sem_node);

        tcb->sem_status = SEM_TIMEOUT;
        tcb->waited_mutex = NULL;

        if (tcb->status == TASK_BLOCKED)
            tcb->status = TASK_READY;
        Scheduler_Put(tcb);

        tcb = (TCB*) List_Peek_Front(&global_mutex_timeout_list);
    }

}



static inline void MTX_Timeout_Insert(TCB* tcb, uint32_t timeout)
{
    if (!tcb) return;
    if (!timeout) return;

    if (!global_mutex_timeout_list.head)
    {
        List_Insert_Back(&global_mutex_timeout_list, &tcb->list_node);
        tcb->timeout = timeout;
        return;
    }

    List_Node *current_node = global_mutex_timeout_list.head, *prev = NULL;
    uint32_t current_timeout = 0;

    while (current_node && (current_timeout + ((TCB*)current_node->data)->timeout <= timeout))
    {
        current_timeout += ((TCB*)current_node->data)->timeout;
        prev = current_node;
        current_node = current_node->next;
    }


    // Setting delta for the new node
    tcb->timeout = timeout - current_timeout;

    if (!current_node)
        List_Insert_After(&global_mutex_timeout_list, &tcb->list_node, prev);
    else
    {
        List_Insert_Before(&global_mutex_timeout_list, &tcb->list_node, current_node);
        // Next node delta update
        if (current_node) ((TCB*)current_node->data)->timeout -= tcb->timeout;
    }
}

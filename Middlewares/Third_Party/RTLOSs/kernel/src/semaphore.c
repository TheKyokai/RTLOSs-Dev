#include "semaphore.h"
#include "port.h"
#include "scheduler.h"
#include "heap.h"

static List global_sem_timeout_list;

void SEM_Init()
{
    List_Init(&global_sem_timeout_list);
}

static inline void SEM_Timeout_Insert(TCB* tcb, uint32_t timeout);

static inline int SEM_Priority_Comparison(void* current, void* other)
{
    return (int)((TCB*)current)->priority - (int)((TCB*)other)->priority;
}

int SEM_Create(Semaphore_t *handle, uint32_t initial_val, int priority)
{
    if (!handle)    return 1;
    if (priority != SEMAPHORE_FIFO && priority != SEMAPHORE_PRIORITY)   return 3;

    Semaphore* created_semaphore = (Semaphore*) HEAP_Alloc(sizeof(Semaphore));

    if (!created_semaphore)
        return 2;

    created_semaphore->val = initial_val;
    List_Init(&created_semaphore->sem_queue);
    created_semaphore->priority = priority;

    *handle = created_semaphore;

    return 0;
}

SEM_STATUS SEM_Acquire(Semaphore_t semaphore, uint32_t timeout)
{
    if (!semaphore)
        return SEM_ERROR;

    if (timeout == 0)
    {
        if (semaphore->val == 0)
            return SEM_TIMEOUT;
        semaphore->val--;
        return SEM_ACQUIRED;
    }

    if (semaphore->val == 0)
    {
        if (timeout < PORT_MAX_TIMEOUT)
        {
            TCB_Current->timed_wait = 1;
            SEM_Timeout_Insert(TCB_Current, timeout);
        }
        TCB_Current->status = TASK_BLOCKED;
        TCB_Current->waited_sem = semaphore;
        if (semaphore->priority)
            List_Insert_Sorted(&semaphore->sem_queue, &TCB_Current->sem_node, SEM_Priority_Comparison);
        else
            List_Insert_Back(&semaphore->sem_queue, &TCB_Current->sem_node);
        Port_Yield();
    }
    else
    {
        --semaphore->val;
        TCB_Current->sem_status = SEM_ACQUIRED;
    }

    return TCB_Current->sem_status;
}

void SEM_Release(Semaphore_t semaphore)
{
    if (!semaphore) return;

    if (!semaphore->sem_queue.head)
    {
        semaphore->val++;
        return;
    }

    TCB* ready_tcb = (TCB*) List_Remove_Front(&semaphore->sem_queue);
    ready_tcb->sem_status = SEM_ACQUIRED;
    ready_tcb->waited_sem = NULL;


    if (ready_tcb->timed_wait)
    {
        ready_tcb->timed_wait = 0;
        // Adjust time delta for next waiting tcb
        if (ready_tcb->list_node.next)
            ((TCB*) ready_tcb->list_node.next->data)->timeout += ready_tcb->timeout;
        ready_tcb->timeout = 0;
        List_Remove(&global_sem_timeout_list, &ready_tcb->list_node);
    }

    if (ready_tcb->status == TASK_BLOCKED)
        ready_tcb->status = TASK_READY;
    Scheduler_Put(ready_tcb);
}

int SEM_Delete(Semaphore_t semaphore)
{
    if (!semaphore) return 1;

    while (semaphore->sem_queue.head)
    {
        TCB* tcb = (TCB*) List_Remove_Front(&semaphore->sem_queue);

        if (tcb->timed_wait)
        {
            tcb->timed_wait = 0;
            // Adjust time delta for next waiting tcb
            if (tcb->list_node.next)
                ((TCB*) tcb->list_node.next->data)->timeout += tcb->timeout;
            tcb->timeout = 0;
            List_Remove(&global_sem_timeout_list, &tcb->list_node);
        }

        tcb->sem_status = SEM_DEAD;
        tcb->waited_sem = NULL;
        if (tcb->status == TASK_BLOCKED)
            tcb->status = TASK_READY;
        Scheduler_Put(tcb);
        
    }
    HEAP_Free(semaphore);
    return 0;
}


// API

int Semaphore_Create(Semaphore_t *handle, uint32_t initial_val, int priority)
{
    struct SEM_Create_args args = { handle, initial_val, priority };
    return (int) Port_Syscall(SVC_SEMAPHORE_CREATE, &args);
}

SEM_STATUS Semaphore_Acquire(Semaphore_t semaphore, uint32_t timeout)
{
    struct SEM_Acquire_args args = { semaphore, timeout };
    SEM_STATUS ret_stat = (SEM_STATUS) Port_Syscall(SVC_SEMAPHORE_ACQUIRE, &args);
    if (timeout != 0)
    {
        ret_stat = Task_Current_Task->sem_status;
        Task_Current_Task->sem_status = SEM_NONE;
    }
    return ret_stat;
}

uint32_t Semaphore_Get_Value(Semaphore_t semaphore)
{
    if (!semaphore) return 0;
    return semaphore->val;
}

void Semaphore_Release(Semaphore_t semaphore)
{
    struct SEM_Release_args args = { semaphore };
    Port_Syscall(SVC_SEMAPHORE_RELEASE, &args);
}

int Semaphore_Delete(Semaphore_t semaphore)
{
    struct SEM_Delete_args args = { semaphore };
    return (int) Port_Syscall(SVC_SEMAPHORE_DELETE, &args);
}

// Called from TCB_SysTick_Tick
void SEM_Tick_Update()
{
    if (!global_sem_timeout_list.head)  return;

    TCB* tcb = (TCB*) List_Peek_Front(&global_sem_timeout_list);
    --tcb->timeout;
    while (tcb && tcb->timeout == 0)
    {
        // Remove from timeout list
        List_Remove_Front(&global_sem_timeout_list);
        // Remove from semaphore queue
        List_Remove(&tcb->waited_sem->sem_queue, &tcb->sem_node);

        tcb->sem_status = SEM_TIMEOUT;
        tcb->waited_sem = NULL;

        if (tcb->status == TASK_BLOCKED)        
            tcb->status = TASK_READY;
        Scheduler_Put(tcb);
        
        tcb = (TCB*) List_Peek_Front(&global_sem_timeout_list);
    }

}

static inline void SEM_Timeout_Insert(TCB* tcb, uint32_t timeout)
{
    if (!tcb) return;
    if (!timeout) return;

    if (!global_sem_timeout_list.head)
    {
        List_Insert_Back(&global_sem_timeout_list, &tcb->list_node);
        tcb->timeout = timeout;
        return;
    }

    List_Node *current_node = global_sem_timeout_list.head, *prev = NULL;
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
        List_Insert_After(&global_sem_timeout_list, &tcb->list_node, prev);
    else
    {
        List_Insert_Before(&global_sem_timeout_list, &tcb->list_node, current_node);
        // Next node delta update
        if (current_node) ((TCB*)current_node->data)->timeout -= tcb->timeout;
    }
}


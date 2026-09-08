#include "mutex.h"
#include "port.h"
#include "scheduler.h"
#include "heap.h"

static List global_mutex_timeout_list;

void Mutex_Init()
{
    List_Init(&global_mutex_timeout_list);
}

int Mutex_Create(Mutex_t *handle, uint32_t initial_val)
{
    if (!handle)    return 1;
    Mutex* created_mutex = (Mutex*) Port_Alloc();

    if (!created_mutex)
        return 2;

    created_mutex->val = initial_val;
    List_Init(&created_mutex->mutex_queue);
    created_mutex->holder = NULL;

    *handle = created_mutex;

    return 0;
}


static inline void Mutex_Timeout_Insert(TCB* tcb, uint32_t timeout);

SEM_STATUS Mutex_Acquire(Mutex_t mutex, uint32_t timeout)
{
    if (!mutex)
        return SEM_ERROR;

    Port_Disable_Interrupts();

    if (mutex->val == 0)
    {
        if (timeout == 0)
            TCB_Current->sem_status = SEM_TIMEOUT;
        else
        {
            if (timeout < PORT_MAX_TIMEOUT)
            {
                TCB_Current->timed_wait = 1;
                Mutex_Timeout_Insert(TCB_Current, timeout);
            }
            TCB_Current->status = TASK_BLOCKED;
            TCB_Current->waited_mutex = mutex;
            List_Insert_Back(&mutex->mutex_queue, &TCB_Current->sem_node);
            Port_Yield();
        }
    }
    else
    {
        --mutex->val;
        mutex->holder = TCB_Current;
        TCB_Current->sem_status = SEM_ACQUIRED;
    }

    Port_Enable_Interrupts();

    return TCB_Current->sem_status;
}

void Mutex_Release(Mutex_t mutex)
{
    if (!mutex) return;

    Port_Disable_Interrupts();

    if (TCB_Current != mutex->holder)
    {
        Port_Enable_Interrupts();
        return;
    }

    if (List_Empty(&mutex->mutex_queue))
    {
        mutex->val++;
        mutex->holder = NULL;
        Port_Enable_Interrupts();
        return;
    }

    TCB* ready_tcb = (TCB*) List_Remove_Front(&mutex->mutex_queue);
    ready_tcb->sem_status = SEM_ACQUIRED;
    ready_tcb->waited_mutex = NULL;
    mutex->holder = ready_tcb;


    if (ready_tcb->timed_wait)
    {
        ready_tcb->timed_wait = 0;
        // Adjust time delta for next waiting tcb
        if (ready_tcb->list_node.next)
            ((TCB*) ready_tcb->list_node.next->data)->timeout += ready_tcb->timeout;
        ready_tcb->timeout = 0;
        List_Remove(&global_mutex_timeout_list, &ready_tcb->list_node);
    }

    ready_tcb->status = TASK_READY;
    Scheduler_Put(ready_tcb);

    Port_Enable_Interrupts();
}

// Called from Task_SysTick_Tick -> other interrupt using RTLOSs API must be masked
void Mutex_Tick_Update()
{
    if (List_Empty(&global_mutex_timeout_list))  return;

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

        tcb->status = TASK_READY;
        Scheduler_Put(tcb);

        tcb = (TCB*) List_Peek_Front(&global_mutex_timeout_list);
    }

}



static inline void Mutex_Timeout_Insert(TCB* tcb, uint32_t timeout)
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

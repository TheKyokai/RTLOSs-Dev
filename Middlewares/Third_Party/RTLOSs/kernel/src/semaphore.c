#include "semaphore.h"
#include "port.h"
#include "scheduler.h"
#include "heap.h"

static List global_sem_timeout_list;

void Semaphore_Init()
{
    List_Init(&global_sem_timeout_list);
}

int Semaphore_Create(Semaphore_t *handle, uint32_t initial_val)
{
    if (!handle)    return 1;
    Semaphore* created_semaphore = (Semaphore*) Port_Alloc();

    if (!created_semaphore)
        return 2;
    
    created_semaphore->val = initial_val;
    List_Init(&created_semaphore->sem_queue);

    *handle = created_semaphore;

    return 0;
}


static inline void Semaphore_Timeout_Insert(TCB* tcb, uint32_t timeout);

SEM_STATUS Semaphore_Acquire(Semaphore_t semaphore, uint32_t timeout)
{
    if (!semaphore)
        return SEM_ERROR;

    Port_Disable_Interrupts();

    if (semaphore->val == 0)
    {
        if (timeout == 0)
            TCB_Current->sem_status = SEM_TIMEOUT;
        else 
        {
            if (timeout < PORT_MAX_TIMEOUT)
            {
                TCB_Current->timed_wait = 1;
                Semaphore_Timeout_Insert(TCB_Current, timeout);
            }
            TCB_Current->status = TASK_BLOCKED;
            TCB_Current->waited_sem = semaphore;
            List_Insert_Back(&semaphore->sem_queue, &TCB_Current->sem_node);
            Port_Yield();
        }
    }
    else 
    {
        --semaphore->val;
        TCB_Current->sem_status = SEM_ACQUIRED;
    }

    Port_Enable_Interrupts();

    return TCB_Current->sem_status;
}

void Semaphore_Release(Semaphore_t semaphore)
{
    if (!semaphore) return;

    Port_Disable_Interrupts();

    if (List_Empty(&semaphore->sem_queue))
    {
        semaphore->val++;
        Port_Enable_Interrupts();
        return;
    }

    TCB* ready_tcb = (TCB*) List_Remove_Front(&semaphore->sem_queue);
    ready_tcb->sem_status = SEM_ACQUIRED;
    ready_tcb->waited_sem = NULL;


    if (ready_tcb->timed_wait)
    {
        ready_tcb->timed_wait = 0; // TODO => Possibly move to SEM_STATUS
        // Adjust time delta for next waiting tcb
        if (ready_tcb->list_node.next)
            ((TCB*) ready_tcb->list_node.next->data)->timeout += ready_tcb->timeout;
        ready_tcb->timeout = 0;
        List_Remove(&global_sem_timeout_list, &ready_tcb->list_node);
    }

    ready_tcb->status = TASK_READY;
    Scheduler_Put(ready_tcb);
    
    Port_Enable_Interrupts();
}

// Called from Task_SysTick_Tick -> other interrupt using RTLOSs API must be masked
void Semaphore_Tick_Update()
{
    if (List_Empty(&global_sem_timeout_list))  return;

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

        tcb->status = TASK_READY;
        Scheduler_Put(tcb);

        tcb = (TCB*) List_Peek_Front(&global_sem_timeout_list);
    }

}



static inline void Semaphore_Timeout_Insert(TCB* tcb, uint32_t timeout)
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


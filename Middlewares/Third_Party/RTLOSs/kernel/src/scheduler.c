#include "scheduler.h"
#include "port.h"
#include "stddef.h"
#include "RTLOSs_config.h"

static Scheduler scheduler;
static Task_t idle_task_handle;

void Scheduler_Init()
{
    scheduler.ready_bitmap = 0;
    for (int i=0; i<config_TASK_PRIORITY_COUNT; i++)
        List_Init(&scheduler.ready_queues[i]);
    List_Init(&scheduler.asleep_queue);
}

void Scheduler_Start()
{
    Task_Create_Task(&idle_task_handle, Idle_Task_Function, NULL, config_IDLE_TASK_PRIORITY, NULL, NULL, 0);
    TCB_Current = Scheduler_Get();
    Port_Start_Scheduler();
}


TCB* Scheduler_Get()
{
    if (scheduler.ready_bitmap == 0)
        return NULL;    // Unexpected behavior -> may mean zero ready tasks, which should never happen
    
    int highest_ready_priority = __builtin_ctz(scheduler.ready_bitmap);
    TCB* ready_task = (TCB*) List_Remove_Front(&scheduler.ready_queues[highest_ready_priority]);
    if (!scheduler.ready_queues[highest_ready_priority].head)   // For some reason using List_Empty does not work
        scheduler.ready_bitmap &= ~(1U << highest_ready_priority);
    return ready_task;
}


void Scheduler_Put(TCB* tcb)
{
    if (!tcb)   return;

    if (tcb->priority > config_MAX_TASK_PRIORITY)
        return;
    List_Insert_Back(&scheduler.ready_queues[tcb->priority], &tcb->list_node);
    scheduler.ready_bitmap |= (1U << tcb->priority);
}


void Scheduler_Sleep_Update()
{
    if (!scheduler.asleep_queue.head)  return;

    TCB* tcb = (TCB*) List_Peek_Front(&scheduler.asleep_queue);
    tcb->timeout--;
    while (tcb && tcb->timeout == 0)
    {
        List_Remove_Front(&scheduler.asleep_queue);
        if (tcb->status == TASK_BLOCKED)
            tcb->status = TASK_READY;
        Scheduler_Put(tcb);
        tcb = (TCB*) List_Peek_Front(&scheduler.asleep_queue);
    }
}

void Scheduler_Sleep_Put(TCB* tcb, uint32_t period)
{
    if (!tcb) return;
    if (!period) return;

    tcb->status = TASK_BLOCKED;
    
    if (!scheduler.asleep_queue.head)
    {
        List_Insert_Back(&scheduler.asleep_queue, &tcb->list_node);
        tcb->timeout = period;
        return;
    }

    List_Node *current_node = scheduler.asleep_queue.head, *prev = NULL;
    uint32_t current_sleep_ticks = 0;
    
    while (current_node && (current_sleep_ticks + ((TCB*)current_node->data)->timeout <= period))
    {
        current_sleep_ticks += ((TCB*)current_node->data)->timeout;
        prev = current_node;
        current_node = current_node->next;
    }


    // Setting delta for the new node
    tcb->timeout = period - current_sleep_ticks;

    if (!current_node)
        List_Insert_After(&scheduler.asleep_queue, &tcb->list_node, prev);
    else
    {
        List_Insert_Before(&scheduler.asleep_queue, &tcb->list_node, current_node);
        // Next node delta update
        if (current_node) ((TCB*)current_node->data)->timeout -= tcb->timeout;
    }
    
}   

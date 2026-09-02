#include "scheduler.h"
#include "port.h"
#include "stddef.h"

static Scheduler scheduler;
static Task_t idle_task_handle;

void Scheduler_Init()
{
    List_Init(&scheduler.ready_list);
    List_Init(&scheduler.asleep_list);
}

void Scheduler_Start()
{
    Task_Create_Task(&idle_task_handle, Idle_Task_Function, NULL, NULL, NULL, 0);
    TCB_Current = Scheduler_Get();
    Port_Start_Scheduler();
}


TCB* Scheduler_Get()
{
    return (TCB*) List_Remove_Front(&scheduler.ready_list);
}


void Scheduler_Put(TCB* tcb)
{
    List_Insert_Back(&scheduler.ready_list, &tcb->list_node);
}


void Scheduler_Sleep_Update()
{
    if (List_Empty(&scheduler.asleep_list))  return;

    TCB* tcb = (TCB*) List_Peek_Front(&scheduler.asleep_list);
    tcb->sleep_ticks_remaining--;
    while (tcb && tcb->sleep_ticks_remaining == 0)
    {
        List_Remove_Front(&scheduler.asleep_list);
        tcb->status = TASK_READY;
        Scheduler_Put(tcb);
        tcb = (TCB*) List_Peek_Front(&scheduler.asleep_list);
    }
}

void Scheduler_Sleep_Put(TCB* tcb, uint32_t period)
{
    if (!tcb) return;
    if (!period) return;

    tcb->status = TASK_BLOCKED;
    
    if (!scheduler.asleep_list.head)
    {
        List_Insert_Back(&scheduler.asleep_list, &tcb->list_node);
        tcb->sleep_ticks_remaining = period;
        return;
    }

    List_Node *current_node = scheduler.asleep_list.head, *prev = NULL;
    uint32_t current_sleep_ticks = 0;
    
    while (current_node && (current_sleep_ticks + ((TCB*)current_node->data)->sleep_ticks_remaining <= period))
    {
        current_sleep_ticks += ((TCB*)current_node->data)->sleep_ticks_remaining;
        prev = current_node;
        current_node = current_node->next;
    }


    // Setting delta for the new node
    tcb->sleep_ticks_remaining = period - current_sleep_ticks;

    if (!current_node)
        List_Insert_After(&scheduler.asleep_list, &tcb->list_node, prev);
    else
    {
        List_Insert_Before(&scheduler.asleep_list, &tcb->list_node, current_node);
        // Next node delta update
        if (current_node) ((TCB*)current_node->data)->sleep_ticks_remaining -= tcb->sleep_ticks_remaining;
    }
    
}   

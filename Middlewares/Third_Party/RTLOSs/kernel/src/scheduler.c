#include "scheduler.h"
#include "port.h"
#include "stddef.h"

static Scheduler scheduler;
static Task_t idle_task_handle;

void Scheduler_Init()
{
    List_Init(&scheduler.list);
}

void Scheduler_Start()
{
    Task_Create_Task(&idle_task_handle, Idle_Task_Function, NULL, NULL, NULL, 0);
    List_Insert_Back(&scheduler.list, &idle_task_handle->list_node);
    TCB_Current = Scheduler_Get();
    Port_Start_Scheduler();
}


TCB* Scheduler_Get()
{
    return (TCB*) List_Remove_Front(&scheduler.list);
}


void Scheduler_Put(TCB* tcb)
{
    List_Insert_Back(&scheduler.list, &tcb->list_node);
}
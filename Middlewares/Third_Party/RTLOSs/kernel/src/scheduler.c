#include "scheduler.h"
#include "task.h"
#include "port.h"

static Scheduler scheduler;
static Task_t idle_task_handle;

void Scheduler_Init()
{
    List_Init(&scheduler.list);
}

void Scheduler_Start()
{
    Task_Create_Task(&idle_task_handle, Idle_Task_Function, NULL, NULL, NULL, NULL);
    List_Insert_Back(&scheduler.list, &idle_task_handle->list_node);
    Start_Task_Execution();
}


TCB* Scheduler_Get()
{
    return (TCB*) List_Remove_Front(&scheduler.list);
}


void Scheduler_Put(TCB* tcb)
{
    List_Insert_Back(&scheduler.list, &tcb->list_node);
}
#include "../inc/scheduler.h"
#include "../inc/task.h"


static Scheduler scheduler;


void init_Scheduler()
{
    List_Init(&scheduler.list);
}


TCB* Scheduler_Get()
{
    return (TCB*) List_Remove_Front(&scheduler.list);
}


void Scheduler_Put(TCB* tcb)
{
    List_Insert_Back(&scheduler.list, &tcb->list_node);
}
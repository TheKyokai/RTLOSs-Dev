#include "task.h"
#include "heap.h"
#include "scheduler.h"
#include "port.h"


TCB* TCB_Current = NULL;

void TCB_Switch_Current()
{
    Scheduler_Put(TCB_Current);
    TCB_Current = Scheduler_Get();
}

void TCB_Task_Function_Wrapper(TCB* tcb)
{
    tcb->task_function(tcb->task_param);
    Task_Delete(tcb);
    while(1); // Infinite loop while waiting for context switch
}


int Task_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags)
{
    if (!task_function)
        return 3;    
    
    TCB* created_TCB = (TCB*) Port_Alloc();
    if (!created_TCB)
        return 1;
    
    void* sp = Port_Alloc();
    if (!sp)
    {
        Port_Free(created_TCB);
        return 2;
    }
    
    created_TCB->saved_sp = ( (uint8_t*) sp + HEAP_BLOCK_SIZE );

    created_TCB->task_function = task_function;
    created_TCB->task_param = task_param;
    created_TCB->hook = hook_function;
    created_TCB->hook_param = hook_param;
    created_TCB->hook_call_flags = hook_flags;
    created_TCB->status = TASK_READY;

    created_TCB->list_node.next = NULL;
    created_TCB->list_node.prev = NULL;
    created_TCB->list_node.data = (void *) created_TCB;

    Init_Task_Stack(created_TCB);

    *handle = created_TCB;
    return 0;
}


int Task_Delete(TCB* tcb)
{
    tcb->status = TASK_DELETED;
    if (tcb == TCB_Current)
        // Trigger context switch
        return 1;
    return 0;
}

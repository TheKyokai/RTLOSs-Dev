#include "task.h"
#include "heap.h"
#include "scheduler.h"
#include "port.h"
#include "RTLOSs_config.h"


void Scheduler_Sleep_Put(TCB* tcb, uint32_t period); // Private declaration -> used only in this file


TCB* TCB_Current = NULL;
static uint32_t Current_TCB_Tick_Count = 0;

void TCB_Switch_Current()
{
    if (TCB_Current->status == TASK_READY)
        Scheduler_Put(TCB_Current);
    TCB_Current = Scheduler_Get();
    Current_TCB_Tick_Count = 0;
}

void TCB_Task_Function_Wrapper(TCB* tcb)
{
    tcb->task_function(tcb->task_param);
    Task_Delete(tcb);
    while(1); // Infinite loop while waiting for context switch
}

int TCB_Initial_Setup(TCB* tcb, void* sp, Task_Function* task_function, void* task_param, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags)
{
    if (!tcb || !sp)
        return 1;

    tcb->saved_sp = (uint32_t*) ( (uint8_t*) sp + HEAP_BLOCK_SIZE ); // TODO => Make stack allocation part of port

    tcb->task_function = task_function;
    tcb->task_param = task_param;
    tcb->hook = hook_function;
    tcb->hook_param = hook_param;
    tcb->hook_call_flags = hook_flags;
    tcb->status = TASK_READY;
    tcb->timeout = 0;

    tcb->list_node.next = NULL;
    tcb->list_node.prev = NULL;
    tcb->list_node.data = (void *) tcb;

    tcb->sem_node.next = NULL;
    tcb->sem_node.prev = NULL;
    tcb->sem_node.data = (void *) tcb;

    tcb->waited_sem = NULL;
    tcb->timed_wait = 0;

    Port_Init_Task_Stack(tcb);
    return 0;
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
    

    TCB_Initial_Setup(created_TCB, sp, task_function, task_param, hook_function, hook_param, hook_flags);
    
    // created_TCB->saved_sp = (uint32_t*) ( (uint8_t*) sp + HEAP_BLOCK_SIZE ); // TODO => Make stack allocation part of port

    // created_TCB->task_function = task_function;
    // created_TCB->task_param = task_param;
    // created_TCB->hook = hook_function;
    // created_TCB->hook_param = hook_param;
    // created_TCB->hook_call_flags = hook_flags;
    // created_TCB->status = TASK_READY;
    // created_TCB->timeout = 0;

    // created_TCB->list_node.next = NULL;
    // created_TCB->list_node.prev = NULL;
    // created_TCB->list_node.data = (void *) created_TCB;

    // created_TCB->sem_node.next = NULL;
    // created_TCB->sem_node.prev = NULL;
    // created_TCB->sem_node.data = (void *) created_TCB;

    // created_TCB->waited_sem = NULL;
    // created_TCB->timed_wait = 0;

    // Port_Init_Task_Stack(created_TCB);

    Scheduler_Put(created_TCB);

    *handle = created_TCB;
    return 0;
}


int Task_Delete(Task_t tcb)
{
    tcb->status = TASK_DELETED;
    if (tcb == TCB_Current)
        // Trigger context switch
        return 1;
    return 0;
}


int Task_Yield()
{
    Port_Yield();
    return 0;
}

void Idle_Task_Function(void * dummy)
{
    while (1)
    {
        // Cleanup memory => TODO
        __asm__ volatile ("wfi"); // Wait for interrupt => Move to port??
    }
}

int Task_SysTick_Tick()
{   
    Scheduler_Sleep_Update();
    Semaphore_Tick_Update();
    ++Current_TCB_Tick_Count;
    if (Current_TCB_Tick_Count >= config_TASK_TICK_TIMESLICE)
        return 1;
    return 0;
}

int Task_Sleep(uint32_t period)
{   
    Port_Disable_Interrupts();
    
    Scheduler_Sleep_Put(TCB_Current, period);
    Task_Yield();   // Will trigger context switch regardless
    
    Port_Enable_Interrupts();
    return 0;
}
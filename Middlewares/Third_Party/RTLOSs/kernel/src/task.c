#include "task.h"
#include "heap.h"
#include "scheduler.h"
#include "port.h"
#include "RTLOSs_config.h"
#include "hook.h"


void Scheduler_Sleep_Put(TCB* tcb, uint32_t period); // Private declaration


TCB* TCB_Current = NULL;
static uint32_t Current_TCB_Tick_Count = 0;
static volatile uint32_t Uptime_Tick_Count = 0;

void TCB_Switch_Current()
{
    
    if (TCB_Current->status == TASK_READY)
        Scheduler_Put(TCB_Current);
    if (TCB_Current->status == TASK_DELETED)
    {
        Hook_Notify(TCB_Current, TASK_HOOK_END_FLAG);
        TCB_TryFree(TCB_Current);
    }
    else
        Hook_Notify(TCB_Current, TASK_HOOK_SWITCH_OUT_FLAG);

    TCB_Current = Scheduler_Get();
    while (TCB_Current)
    {
        // TASK_BLOCKED should never be in the scheduler
        if (TCB_Current->status == TASK_READY)
            break;
        if (TCB_Current->status == TASK_DELETED)
        {
            Hook_Notify(TCB_Current, TASK_HOOK_END_FLAG);
            TCB_TryFree(TCB_Current);
        }
        TCB_Current = Scheduler_Get();
    }
    Hook_Notify(TCB_Current, TASK_HOOK_SWITCH_IN_FLAG);
    Current_TCB_Tick_Count = 0;
    // TCB_Current should never be NULL since the idle task is always ready
}

uint32_t Task_Get_Tick_Count()
{
    return Uptime_Tick_Count;
}

void TCB_Task_Function_Wrapper(TCB* tcb)
{
    tcb->task_function(tcb->task_param);
    Task_Delete(tcb);
    while(1); // Infinite loop while waiting for context switch => Unexpected function return
}

int TCB_Initial_Setup(TCB* tcb, void* sp, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags)
{
    if (!tcb || !sp)
        return 1;

    tcb->saved_sp = (uint32_t*) ( (uint8_t*) sp + config_DEFAULT_STACK_SIZE );
    tcb->allocated_stack_start = sp;

    tcb->task_function = task_function;
    tcb->task_param = task_param;
    tcb->hook = hook_function;
    tcb->hook_param = hook_param;
    tcb->hook_call_flags = hook_flags;
    tcb->status = TASK_READY;
    tcb->timeout = 0;
    tcb->priority = priority;

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


int TCB_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags)
{
    if (!task_function)
        return 3;
    
    if (priority > config_MAX_TASK_PRIORITY)
        return 4;
    
    TCB* created_TCB = (TCB*) HEAP_Alloc(sizeof(TCB));
    if (!created_TCB)
        return 1;

    void* sp = HEAP_Alloc(config_DEFAULT_STACK_SIZE);
    if (!sp)
    {
        HEAP_Free(created_TCB);
        return 2;
    }
    

    TCB_Initial_Setup(created_TCB, sp, task_function, task_param, priority, hook_function, hook_param, hook_flags);
    Scheduler_Put(created_TCB);

    *handle = created_TCB;
    return 0;
}

int TCB_Delete(Task_t tcb)
{
    tcb->status = TASK_DELETED;
    if (tcb == TCB_Current)
        Port_Yield();
    return 0;
}

int TCB_SysTick_Tick()
{
    Scheduler_Sleep_Update();
    SEM_Tick_Update();
    MTX_Tick_Update();
    ++Uptime_Tick_Count;
    ++Current_TCB_Tick_Count;
    if (Current_TCB_Tick_Count >= config_TASK_TICK_TIMESLICE)
        return 1;
    return 0;
}

int TCB_Sleep(uint32_t period)
{
    Scheduler_Sleep_Put(TCB_Current, period);
    if (period > 0)
        Port_Yield();
    return 0;
}


// API - SVCall Wrappers

int Task_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags)
{
    struct TCB_Create_args args = { handle, task_function, task_param, priority, hook_function, hook_param, hook_flags };
    return (int) Port_Syscall(SVC_TASK_CREATE, &args);
}

int Task_Delete(Task_t tcb)
{
    struct TCB_Delete_args args = { tcb };
    return (int) Port_Syscall(SVC_TASK_DELETE, &args);
}

int Task_Yield()
{
    Port_Yield();
    return 0;
}

int Task_Sleep(uint32_t period)
{
    struct TCB_Sleep_args args = { period };
    return (int) Port_Syscall(SVC_TASK_SLEEP, &args);
}


void Idle_Task_Function(void * dummy)
{
    while (1)
    {
        Port_WFI();
    }
}


void TCB_TryFree(TCB* tcb)
{
    if (!tcb)
        return;

    if (tcb->owned_mutex_cnt > 0)
        return;

    HEAP_Free(tcb->allocated_stack_start);
    HEAP_Free(tcb);
}

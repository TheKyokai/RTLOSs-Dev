#include "timer.h"
#include "heap.h"
#include "scheduler.h"
#include "RTLOSs_config.h"
#include "port.h"



void Timer_Task_Wrapper(void* timer_param)
{
    Timer_t timer = (Timer_t) timer_param;
    while (timer->status != TIMER_STOPPED)
    {
        timer->function(timer->param);
        Task_Sleep(timer->period);
    }
    Heap_Free(timer);
}

int TIM_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period)
{
    if (!handle || !timer_function)
        return 1;

    Timer* created_timer = (Timer*) HEAP_Alloc(sizeof(Timer));
    if (!created_timer)
        return 2;

    created_timer->timer_task = (TCB*) HEAP_Alloc(sizeof(TCB));
    if (!created_timer->timer_task)
    {
        HEAP_Free(created_timer);
        return 3;
    }

    void* sp = HEAP_Alloc(config_DEFAULT_STACK_SIZE);
    if (!sp)
    {
        HEAP_Free(created_timer->timer_task);
        HEAP_Free(created_timer);
        return 4;
    }

    created_timer->function = timer_function;
    created_timer->param = timer_param;
    created_timer->period = period;
    created_timer->status = TIMER_STARTED;

    TCB_Initial_Setup(created_timer->timer_task, sp, Timer_Task_Wrapper, created_timer, config_TIMER_TASK_PRIORITY, NULL, NULL, 0);

    Scheduler_Put(created_timer->timer_task);

    *handle = created_timer;

    return 0;
}

int Timer_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period)
{
    struct TIM_Create_args args = { handle, timer_function, timer_param, period };
    return (int) Port_Syscall(SVC_TIMER_CREATE, &args);
}


int Timer_Delete(Timer_t handle)
{
    handle->status = TIMER_STOPPED;
    return 0;
}
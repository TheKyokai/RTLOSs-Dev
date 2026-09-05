#include "timer.h"
#include "heap.h"
#include "scheduler.h"



void Timer_Task_Wrapper(void* timer_param)
{
    Timer_t timer = (Timer_t) timer_param;
    while (timer->status != TIMER_STOPPED)
    {
        timer->function(timer->param);
        Task_Sleep(timer->period);
    }
}

int Timer_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period)
{
    if (!handle || !timer_function)
        return 1;
    
    Timer* created_timer = (Timer*) Port_Alloc();
    if (!created_timer)
        return 2;
    
    void* sp = Port_Alloc();
    if (!sp)
    {
        Port_Free(created_timer);
        return 3;
    }

    created_timer->function = timer_function;
    created_timer->param = timer_param;
    created_timer->period = period;
    created_timer->status = TIMER_STARTED;

    TCB_Initial_Setup(&created_timer->timer_task, sp, Timer_Task_Wrapper, created_timer, NULL, NULL, 0);

    Scheduler_Put(&created_timer->timer_task);

    *handle = created_timer;

    return 0;
}


int Timer_Delete(Timer_t handle)
{
    handle->status = TIMER_STOPPED;
    return 0;
}
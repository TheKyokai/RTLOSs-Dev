#ifndef TIMER_H
#define TIMER_H

#include "enum_defs.h"
#include "task.h"
#include "stdint.h"

/*
    API     - Timer_*
    SYSCALL - TIM_*
*/


typedef void Timer_Function(void*);

typedef struct Timer Timer;
typedef Timer* Timer_t;

struct Timer
{
    Timer_Function* function;
    void* param;
    uint32_t period;
    TIMER_STATUS status;

    Task_t timer_task;
};

// Part of the API
int Timer_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period);
int Timer_Delete(Timer_t handle);


// Used by SysCall

int TIM_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period);
// Timer_Delete has no SysCall counterpart - only sets a status flag, no shared state to protect


// Syscall arg structs

struct TIM_Create_args
{
        Timer_t* handle;
        Timer_Function* timer_function;
        void* timer_param;
        uint32_t period;
};


#endif
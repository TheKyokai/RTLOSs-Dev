#ifndef TIMER_H
#define TIMER_H

#include "enum_defs.h"
#include "task.h"
#include "stdint.h"


typedef void Timer_Function(void*);

typedef struct Timer Timer;
typedef Timer* Timer_t;

struct Timer
{
    Timer_Function* function;
    void* param;
    uint32_t period;
    TIMER_STATUS status;

    TCB timer_task;
};


int Timer_Create_Timer(Timer_t* handle, Timer_Function* timer_function, void* timer_param, uint32_t period);
int Timer_Delete(Timer_t handle);


#endif
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "list.h"
#include "task.h"

typedef struct Scheduler Scheduler;

struct Scheduler
{
    List ready_list;
    List asleep_list;
};


void Scheduler_Init();
void Scheduler_Start();
TCB* Scheduler_Get();
void Scheduler_Put(TCB* tcb);

void Scheduler_Sleep_Update();

#endif
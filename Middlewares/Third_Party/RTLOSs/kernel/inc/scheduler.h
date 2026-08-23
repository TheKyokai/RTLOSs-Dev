#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../inc/list.h"

struct TCB;

typedef struct Scheduler
{
    List list;
} Scheduler;


void init_Scheduler();
TCB* Scheduler_Get();
void Scheduler_Put(TCB* tcb);

#endif
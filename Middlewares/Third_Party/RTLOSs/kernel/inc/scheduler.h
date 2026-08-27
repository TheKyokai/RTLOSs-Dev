#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../inc/list.h"

struct TCB;

typedef struct Scheduler
{
    List list;
} Scheduler;


void Scheduler_Init();
void Scheduler_Start();
TCB* Scheduler_Get();
void Scheduler_Put(TCB* tcb);

#endif
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "list.h"
#include "task.h"
#include "RTLOSs_config.h"


// Should NEVER be directly interacted with by the user, NOT A PART OF THE API

typedef struct Scheduler Scheduler;

struct Scheduler
{
    List ready_queues[config_TASK_PRIORITY_COUNT];
    List asleep_queue;
    uint32_t ready_bitmap;
};


void Scheduler_Init();
void Scheduler_Start();
TCB* Scheduler_Get();
void Scheduler_Put(TCB* tcb);

void Scheduler_Sleep_Update();

#endif
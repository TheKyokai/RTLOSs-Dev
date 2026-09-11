#include "scheduler.h"
#include "heap.h"
#include "hook.h"

void RTLOSs_Init()
{
    Heap_Init();
    Scheduler_Init();
    SEM_Init();
    MTX_Init();
    Hook_Init();
}


void RTLOSs_Start()
{
    Scheduler_Start();
}
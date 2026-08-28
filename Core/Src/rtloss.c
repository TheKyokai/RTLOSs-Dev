#include "scheduler.h"
#include "heap.h"

void RTLOSs_Init()
{
    Port_Heap_Init();
    Scheduler_Init();
}


void RTLOSs_Start()
{
    Scheduler_Start();
}
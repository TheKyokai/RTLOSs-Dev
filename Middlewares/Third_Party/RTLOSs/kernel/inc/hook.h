#ifndef HOOK_H
#define HOOK_H

#include "stdint.h"


typedef struct TCB TCB;

void Hook_Init();

// Privileged - called only from TCB_Switch_Current (PendSV context), never blocks or allocates
void Hook_Notify(TCB* tcb, uint8_t event_flag);

#endif

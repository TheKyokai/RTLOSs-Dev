#ifndef PORT_H
#define PORT_H

#include "task.h"


#define INITIAL_XPSR 0x01000000
#define INITIAL_PC_MASK 0xFFFFFFFE
#define INITIAL_LR 0x00000000 // TODO Update for debugging

#define PORT_MAX_TIMEOUT UINT32_MAX


void Port_Init_Task_Stack(TCB* tcb);
void Port_Start_Scheduler();
void Port_Yield();

void Port_Enable_Interrupts();
void Port_Disable_Interrupts();


#endif
#ifndef PORT_H
#define PORT_H


#define INITIAL_XPSR 0x01000000
#define INITIAL_PC_MASK 0xFFFFFFFE
#define INITIAL_LR 0x00000000 // TODO Update for debugging


void Init_Task_Stack(TCB* tcb);

#endif
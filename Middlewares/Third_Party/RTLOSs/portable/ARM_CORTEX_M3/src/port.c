#include "port.h"
#include "task.h"



void PendSV_Handler( void ) __attribute__(( naked ));



void Init_Task_Stack(TCB* tcb)
{
    uint32_t* tcb_sp = tcb->saved_sp;
    tcb_sp--;
    *tcb_sp = INITIAL_XPSR;
    tcb_sp--;
    *tcb_sp = ((uint32_t) TCB_Task_Function_Wrapper) & INITIAL_PC_MASK;
    tcb_sp--;
    *tcb_sp = INITIAL_LR;
    tcb_sp -= 5; // Skipping r12, r3-r1
    *tcb_sp = (uint32_t) tcb;
    tcb_sp -= 8; // Skipping r11-r4
    tcb->saved_sp = tcb_sp;
}


void PendSV_Handler( void )
{
    // Context switch
    __asm__ volatile
    (
        "   mrs r0, psp                             \n"
        "   isb                                     \n"
        "                                           \n"
        "   ldr r2, PendSV_TCB_Current              \n"
        "   ldr r3, [r2]                            \n"     // Load current_TCB saved_pc
        "   stmdb r0!, {r4-r11}                     \n"     // Store non-saved register of the current task
        "   str r0, [r3]                            \n"     // Save sp of the current task
        "                                           \n"
        "                                           \n"
        "                                           \n"
        "   .align 4                                \n"
        "   PendSV_TCB_Current: .word TCB_Current   \n"     // ARM literal 'pool'
    );
}


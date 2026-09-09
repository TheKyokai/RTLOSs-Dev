#ifndef PORT_H
#define PORT_H

#include "task.h"


#define INITIAL_XPSR 0x01000000
#define INITIAL_PC_MASK 0xFFFFFFFE
#define INITIAL_LR 0x00000000 // TODO Update for debugging

#define PORT_MAX_TIMEOUT UINT32_MAX

// Priority
#define PORT_MAX_SYSCALL_PRIORITY 5
#define SVC_PRIORITY 1
#define SYSTICK_PRIORITY 2
#define PENDSV_PRIORITY 15


// SysTick (base: 0xE000E010)
#define SYSTICK_CTRL                (*(volatile uint32_t*) 0xE000E010)
#define SYSTICK_LOAD                (*(volatile uint32_t*) 0xE000E014)
#define SYSTICK_VAL                 (*(volatile uint32_t*) 0xE000E018)

#define SYSTICK_CTRL_ENABLE_MASK     (1U << 0)
#define SYSTICK_CTRL_TICKINT_MASK    (1U << 1)
#define SYSTICK_CTRL_CLKSOURCE_MASK  (1U << 2)

// SCB: Interrupt Control and State Register (base: 0xE000ED04)
#define SCB_ICSR                    (*(volatile uint32_t*) 0xE000ED04)
#define SCB_ICSR_PENDSVSET_Msk      (1U << 28)

// SCB: System Handler Priority (SHPR2/SHPR3)
// Exception numbers: SVCall=11, PendSV=14, SysTick=15
#define SCB_SHPR_SVCALL             (*(volatile uint8_t*) 0xE000ED1F)   // SHPR2[31:24]
#define SCB_SHPR_PENDSV             (*(volatile uint8_t*) 0xE000ED22)   // SHPR3[23:16]
#define SCB_SHPR_SYSTICK            (*(volatile uint8_t*) 0xE000ED23)   // SHPR3[31:24]



void Port_Init_Task_Stack(TCB* tcb);
void Port_Start_Scheduler();
void Port_Yield();

void Port_Enable_Interrupts();
void Port_Disable_Interrupts();


#endif
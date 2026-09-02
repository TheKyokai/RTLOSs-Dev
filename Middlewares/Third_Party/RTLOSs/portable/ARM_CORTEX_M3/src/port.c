#include "port.h"
#include "task.h"
#include "scheduler.h"
#include "stm32f1xx.h"
#include "RTLOSs_config.h"


void PendSV_Handler( void ) __attribute__(( naked ));
static void Start_Task_Execution() __attribute__(( naked ));
void SVC_Handler( void ) __attribute__(( naked ));



void Port_Init_Task_Stack(TCB* tcb)
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


// Task based execution start - Abandons main thread
static void Start_Task_Execution()
{
    __asm__ volatile
    (
        "   ldr r0, =0xE000ED08     \n"     // Locate initial stack via SCB_VTOR
        "   ldr r0, [r0]            \n"
        "   ldr r0, [r0]            \n"
        "   msr msp, r0             \n"     // Reset MSP - Interrupt Handler Stack
        "   cpsie i                 \n"     // Unmask Interrupts
        "   cpsie f                 \n"
        "   dsb                     \n"     // Synchronize
        "   isb                     \n"
        "                           \n"
        "   svc 0                   \n"     // Start initial task
    );
}

// Inital task start
void SVC_Handler( void )
{
    __asm__ volatile
    (
        "   ldr r2, =TCB_Current              \n"
        "   ldr r3, [r2]                            \n"     // Load new SP
        "   ldr r0, [r3]                            \n"     
        "   ldmia r0!, {r4-r11}                     \n"
        "   msr psp, r0                             \n"
        "   isb                                     \n"
        "                                           \n"
        "   mov r0, #0                              \n"     // Initial intr priority
        "   msr basepri, r0                         \n"
        "                                           \n"
        "   orr r14, #0xD                           \n"     // Switch to Thread mode - Setting lowest LR nibble to D
        "   bx r14                                  \n"     // LR return
    );
}

// Context switch interrupt
void PendSV_Handler( void )
{
    __asm__ volatile
    (
        "   mrs r0, psp                             \n"
        "   isb                                     \n"
        "                                           \n"
        "   ldr r2, =TCB_Current              \n"
        "   ldr r3, [r2]                            \n"     // Load current_TCB saved_pc
        "   stmdb r0!, {r4-r11}                     \n"     // Store non-saved register of the current task
        "   str r0, [r3]                            \n"     // Save sp of the current task
        "                                           \n"
        "   stmdb sp!, {r2, r14}                    \n"     // Saving current TCB and LR
        "   mov r0, %0                              \n"     // Ensuring no low priority interrupt takes over
        "   msr basepri, r0                         \n"
        "   bl TCB_Switch_Current                   \n"
        "   mov r0, #0                              \n"
        "   msr basepri, r0                         \n"
        "   ldmia sp!, {r2, r14}                    \n"     // Restoring saved current TCB and LR
        "                                           \n"
        "   ldr r3, [r2]                            \n"     // Load new SP
        "   ldr r0, [r3]                            \n"     
        "   ldmia r0!, {r4-r11}                     \n"
        "   msr psp, r0                             \n"
        "   isb                                     \n"
        "                                           \n"
        "   bx r14                                  \n"     // LR return
        ::"i"(5 << 4) // Max syscall priority => Make macro later
    );
}


inline void Port_Set_BASEPRI(uint32_t pri)
{
    uint32_t basepri_reg;
    __asm__ volatile 
    (
        "mov %0, %1         \n"
        "msr basepri, %0    \n"
        "isb                \n"
        "dsb                \n"
        : "=r"(basepri_reg) : "i"(pri << 4)     // BASEPRI bits 7:4 are used - 3:0 reserved
    );
}

inline void Port_Enable_Interrupts()
{
    Port_Set_BASEPRI(0);
}

inline void Port_Disable_Interrupts()
{
    Port_Set_BASEPRI(5);
}


static void Port_Start_Kernel_Timer()
{
    SysTick->LOAD = ( config_CPU_CLOCK_HZ / config_TICK_HZ) - 1U;
    SysTick->VAL = 0U;
    
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk;
}


void Port_Start_Scheduler()
{
    
    Port_Start_Kernel_Timer();
    
    Start_Task_Execution();
    // Control should never reach this
}



// RTOS timer interrupt
void SysTick_Handler( void )
{
    Port_Disable_Interrupts();
    
    if (Task_SysTick_Tick())
        Port_Yield();       // Context Switch will occur after enabling interrupts
    
    Port_Enable_Interrupts();
}



inline void Port_Yield()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

#include "port.h"
#include "RTLOSs_config.h"
#include "scheduler.h"
#include "task.h"
#include "timer.h"
#include "heap.h"




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

// Portable SVCall wrapper
uint32_t Port_Syscall(uint32_t syscall_code, void* args)
{
    register uint32_t r0 __asm__("r0") = syscall_code;
    register uint32_t r1 __asm__("r1") = (uint32_t) args;
    __asm__ volatile ("svc 1" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

// SVCall Handler
void SVC_Handler( void )
{
    __asm__ volatile
    (   
        "   tst  lr, #4                 \n"     // Checking out which sp was used
        "   ite  eq                     \n"
        "   mrseq r0, msp               \n"
        "   mrsne r0, psp               \n"
        "                               \n"
        "   ldr  r1, [r0, #24]          \n"     // stacked PC
        "   ldrb r1, [r1, #-2]          \n"     // svc immediate 
        "   cmp  r1, #0                 \n"     // 0 => bootstrap, 1 => SVC
        "   beq  bootstrap              \n"
        "   b    SVC_Dispatch           \n"     // r0 - frame pointer
        "                               \n"
        "                               \n"
        "bootstrap:                     \n"     // Task bootstrap
        "   ldr r2, =TCB_Current        \n"
        "   ldr r3, [r2]                \n"     // Load new SP
        "   ldr r0, [r3]                \n"
        "   ldmia r0!, {r4-r11}         \n"
        "   msr psp, r0                 \n"
        "   isb                         \n"
        "                               \n"
        "   mov r0, #0                  \n"     // Initial intr priority
        "   msr basepri, r0             \n"
        "                               \n"
        "   orr r14, #0xD               \n"     // Switch to Thread mode - Setting lowest LR nibble to D
        "   bx  r14                     \n"     // LR return
    );
}


void SVC_Dispatch(uint32_t* frame)
{
    uint32_t svcall_code = frame[0];                // r0
    void* passed_arg_struct = (void*) frame[1];     // r1
    uint32_t result = 0;

    switch (svcall_code)
    {
        case SVC_HEAP_ALLOC:
        {
            struct HEAP_Alloc_args* args = (struct HEAP_Alloc_args*) passed_arg_struct;
            result = (uint32_t) HEAP_Alloc(args->size);
            break;
        }
        case SVC_HEAP_FREE:
        {
            struct HEAP_Free_args* args = (struct HEAP_Free_args*) passed_arg_struct;
            HEAP_Free(args->block);
            break;
        }
        case SVC_TASK_CREATE:
        {    
            // TCB_Create_Task
            struct TCB_Create_args* args = (struct TCB_Create_args*) passed_arg_struct;
            result = (uint32_t) TCB_Create_Task(args->handle, args->task_function, args->task_param, args->priority, args->hook_function, args->hook_param, args->hook_flags);
            break;
        }
        case SVC_TASK_DELETE:
        {
            struct TCB_Delete_args* args = (struct TCB_Delete_args*) passed_arg_struct;
            result = (uint32_t) TCB_Delete(args->tcb);
            break;
        }
        case SVC_TASK_YIELD:
        {
            // Currently a placeholder since pendingSV can be done safely from anywhere
            break;
        }
        case SVC_TASK_SLEEP:
        {
            struct TCB_Sleep_args* args = (struct TCB_Sleep_args*) passed_arg_struct;
            result = (uint32_t) TCB_Sleep(args->period);
            break;
        }
        case SVC_SEMAPHORE_CREATE:
        {
            struct SEM_Create_args* args = (struct SEM_Create_args*) passed_arg_struct;
            result = (uint32_t) SEM_Create(args->handle, args->initial_val, args->priority);
            break;
        }
        case SVC_SEMAPHORE_ACQUIRE:
        {
            struct SEM_Acquire_args* args = (struct SEM_Acquire_args*) passed_arg_struct;
            result = (uint32_t) SEM_Acquire(args->semaphore, args->timeout);
            break;
        }
        case SVC_SEMAPHORE_RELEASE:
        {
            struct SEM_Release_args* args = (struct SEM_Release_args*) passed_arg_struct;
            SEM_Release(args->semaphore);
            break;
        }
        case SVC_SEMAPHORE_DELETE:
        {
            struct SEM_Delete_args* args = (struct SEM_Delete_args*) passed_arg_struct;
            result = (uint32_t) SEM_Delete(args->semaphore);
            break;
        }
        case SVC_MUTEX_CREATE:
        {
            struct MTX_Create_args* args = (struct MTX_Create_args*) passed_arg_struct;
            result = (uint32_t) MTX_Create(args->handle, args->initial_val, args->priority);
            break;
        }
        case SVC_MUTEX_ACQUIRE:
        {
            struct MTX_Acquire_args* args = (struct MTX_Acquire_args*) passed_arg_struct;
            result = (uint32_t) MTX_Acquire(args->mutex, args->timeout);
            break;
        }
        case SVC_MUTEX_RELEASE:
        {
            struct MTX_Release_args* args = (struct MTX_Release_args*) passed_arg_struct;
            MTX_Release(args->mutex);
            break;
        }
        case SVC_MUTEX_DELETE:
        {
            struct MTX_Delete_args* args = (struct MTX_Delete_args*) passed_arg_struct;
            result = (uint32_t) MTX_Delete(args->mutex);
            break;
        }
        case SVC_TIMER_CREATE:
        {
            struct TIM_Create_args* args = (struct TIM_Create_args*) passed_arg_struct;
            result = (uint32_t) TIM_Create_Timer(args->handle, args->timer_function, args->timer_param, args->period);
            break;
        }
    }

    frame[0] = result;  // Will be restored to r0
}





// Context switch interrupt
void PendSV_Handler( void )
{
    __asm__ volatile
    (
        "   mrs r0, psp                             \n"
        "   isb                                     \n"
        "                                           \n"
        "   ldr r2, =TCB_Current                    \n"
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
        ::"i"(SYSTICK_PRIORITY << 4)    // Mask Systick
    );
}


static inline void Port_Set_BASEPRI(uint32_t pri)
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

void Port_Enable_Interrupts()
{
    Port_Set_BASEPRI(0);
}

void Port_Disable_Interrupts()
{
    Port_Set_BASEPRI(PORT_MAX_SYSCALL_PRIORITY);
}



static inline void Port_Interrupt_Priority_Setup()
{
    SCB_SHPR_SVCALL = SVC_PRIORITY << 4;
    SCB_SHPR_PENDSV = PENDSV_PRIORITY << 4;
    SCB_SHPR_SYSTICK = SYSTICK_PRIORITY << 4;
}

static inline void Port_Start_Kernel_Timer()
{
    SYSTICK_LOAD = ( config_CPU_CLOCK_HZ / config_TICK_HZ) - 1U;
    SYSTICK_VAL = 0U;
    
    SYSTICK_CTRL = SYSTICK_CTRL_ENABLE_MASK | SYSTICK_CTRL_TICKINT_MASK | SYSTICK_CTRL_CLKSOURCE_MASK;
}


void Port_Start_Scheduler()
{
    Port_Interrupt_Priority_Setup();
    Port_Start_Kernel_Timer();
    
    Start_Task_Execution();
    // Control should never reach this
}

// volatile uint32_t systick_check = 0;

// RTOS timer interrupt
void SysTick_Handler( void )
{
    // systick_check++;
    if (TCB_SysTick_Tick())
        Port_Yield();       // Context Switch will occur after enabling interrupts   
}



inline void Port_Yield()
{
    SCB_ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


void Port_WFI()
{
    __asm__ volatile ("wfi");
}

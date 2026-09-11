#ifndef TASK_H
#define TASK_H

#include "stdint.h"
#include "list.h"
#include "enum_defs.h"
#include "semaphore.h"
#include "mutex.h"

/*
    API     - Task_*
    SYSCALL - TCB_*
*/

typedef void Task_Function(void *);
typedef void Task_Hook_Function(void *);


#define TASK_HOOK_SWITCH_OUT_FLAG   0x1
#define TASK_HOOK_SWITCH_IN_FLAG    0x10
#define TASK_HOOK_END_FLAG          0x100



typedef struct TCB TCB;

struct TCB
{
    uint32_t* saved_sp;
    
    Task_Function* task_function;
    void* task_param;

    Task_Hook_Function* hook;
    void* hook_param;

    List_Node list_node;
    List_Node sem_node;

    TASK_STATUS status;
    
    uint32_t timeout;
    
    // Priority for scheduling
    uint32_t priority;

    // Semaphore related
    Semaphore* waited_sem; // Reordering causes proteus fatal error
    SEM_STATUS sem_status;
    uint8_t timed_wait;

    // Mutex related
    Mutex* waited_mutex;
    uint32_t owned_mutex_cnt;

    void* allocated_stack_start;
    uint8_t hook_call_flags;
};


typedef TCB* Task_t;

extern TCB* TCB_Current;
#define Task_Current_Task TCB_Current


// Used by other parts of the kernel

void TCB_Switch_Current();
void TCB_Task_Function_Wrapper(TCB* tcb);
int TCB_Initial_Setup(TCB* tcb, void* sp, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags);

void Idle_Task_Function(void * dummy);
int TCB_SysTick_Tick();
void TCB_TryFree(TCB* tcb);


// Part of the API

int Task_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags);
int Task_Delete(Task_t tcb);
int Task_Yield();
int Task_Sleep(uint32_t period);
uint32_t Task_Get_Tick_Count();      // No SysCall counterpart - plain atomic read, no shared state to protect


// Used by SysCall

int TCB_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, uint32_t priority, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags);
int TCB_Delete(Task_t tcb);
int TCB_Sleep(uint32_t period);



// Syscall arg structs

struct TCB_Create_args
{
        Task_t* handle;
        Task_Function* task_function;
        void* task_param;
        uint32_t priority;
        Task_Hook_Function* hook_function;
        void* hook_param;
        uint8_t hook_flags;
};

struct TCB_Delete_args
{
        Task_t tcb;
};

struct TCB_Sleep_args
{
        uint32_t period;
};



#endif
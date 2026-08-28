#ifndef TASK_H
#define TASK_H

#include "stdint.h"
#include "list.h"


typedef void Task_Function(void *);
typedef void Task_Hook_Function(void *);


#define TASK_HOOK_SWITCH_OUT_FLAG   0x1
#define TASK_HOOK_SWITCH_IN_FLAG    0x10
#define TASK_HOOK_END_FLAG          0x100

typedef enum TASK_STATUS
{
    TASK_READY, TASK_BLOCKED, TASK_DELETED
} TASK_STATUS;


typedef struct TCB TCB;

struct TCB
{
    uint32_t* saved_sp;
    
    Task_Function* task_function;
    void* task_param;

    Task_Hook_Function* hook;
    void* hook_param;

    // Add node ptr after implementing scheduler to remove allocation from list
    List_Node list_node;

    TASK_STATUS status;

    uint8_t hook_call_flags;
};


typedef TCB* Task_t;

extern TCB* TCB_Current;
#define Task_Current_Task TCB_Current

void TCB_Switch_Current();
void TCB_Task_Function_Wrapper(TCB* tcb);

int Task_Create_Task(Task_t* handle, Task_Function* task_function, void* task_param, Task_Hook_Function* hook_function, void* hook_param, uint8_t hook_flags);
int Task_Delete(TCB* tcb);
int Task_Yield();

void Idle_Task_Function(void * dummy);

#endif
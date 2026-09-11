#include "hook.h"
#include "task.h"
#include "semaphore.h"
#include "port.h"
#include "RTLOSs_config.h"

typedef struct Hook_Event
{
    Task_Hook_Function* function;
    void* param;
} Hook_Event;

static Hook_Event hook_queue[config_HOOK_QUEUE_SIZE];
static uint32_t hook_head = 0;
static uint32_t hook_tail = 0;
static uint32_t hook_count = 0;

static Semaphore_t hook_signal;
static Task_t hook_runner_task;

static void Hook_Runner(void* dummy)
{
    while (1)
    {
        Semaphore_Acquire(hook_signal, PORT_MAX_TIMEOUT);

        Port_Disable_Interrupts();
        Hook_Event event = hook_queue[hook_head];
        hook_head = (hook_head + 1) % config_HOOK_QUEUE_SIZE;
        hook_count--;
        Port_Enable_Interrupts();

        event.function(event.param);
    }
}

void Hook_Init()
{
    SEM_Create(&hook_signal, 0, SEMAPHORE_FIFO);
    Task_Create_Task(&hook_runner_task, Hook_Runner, NULL, config_HOOK_TASK_PRIORITY, NULL, NULL, 0);
}

void Hook_Notify(TCB* tcb, uint8_t event_flag)
{
    if (!tcb->hook) return;
    if (!(tcb->hook_call_flags & event_flag)) return;

    if (hook_count >= config_HOOK_QUEUE_SIZE)
        return; // Runner can't keep up - drop this notification

    hook_queue[hook_tail].function = tcb->hook;
    hook_queue[hook_tail].param = tcb->hook_param;
    hook_tail = (hook_tail + 1) % config_HOOK_QUEUE_SIZE;
    hook_count++;

    SEM_Release(hook_signal);
}

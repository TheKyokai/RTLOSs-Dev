#include "stm32f1xx.h"

#include "task.h"
#include "port.h"
#include "semaphore.h"
#include "timer.h"


// Task_t task1, task2, task3;
// Semaphore_t sem1;


// void Task_Pin_Toggler_C7(void* dummy)
// {
//     while (1)
//     {
//         HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
//         // Task_Sleep(2000);
//         Semaphore_Acquire(sem1, PORT_MAX_TIMEOUT);
//     }
// }


// void Task_Pin_Toggler_C8(void* dummy)
// {
//     while (1)
//     {
//         HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
//         // Task_Sleep(1000);
//         Semaphore_Acquire(sem1, 1000);
//     }
// }



// void Task_Semaphore_Releaser(void* semaphore)
// {
//     Semaphore_t sem = (Semaphore_t) semaphore;
//     while(1)
//     {
//         Task_Sleep(2000);
//         Semaphore_Release(sem);
//     }

// }




// void test_1()
// {
//     Semaphore_Create(&sem1, 0);
//     Task_Create_Task(&task1, Task_Pin_Toggler_C7, NULL, NULL, NULL, 0);
//     Task_Create_Task(&task2, Task_Pin_Toggler_C8, NULL, NULL, NULL, 0);
//     Task_Create_Task(&task3, Task_Semaphore_Releaser, sem1, NULL, NULL, 0);
// }




Timer_t timer1, timer2;


void Sleepless_Toggler(void* GPIO_PIN)
{
    HAL_GPIO_TogglePin(GPIOC, (uint16_t) GPIO_PIN);
}


void test_2()
{
    Timer_Create_Timer(&timer1, Sleepless_Toggler, (void*) GPIO_PIN_7, 2000);
    Timer_Create_Timer(&timer2, Sleepless_Toggler, (void*) GPIO_PIN_8, 1000);
}




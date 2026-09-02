#include "stm32f1xx.h"

#include "task.h"

void Task_Pin_Toggler_C7(void* dummy)
{
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
        Task_Sleep(2000);
    }
}


void Task_Pin_Toggler_C8(void* dummy)
{
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
        Task_Sleep(1000);
    }
}


Task_t task1, task2;

void test_1()
{
    
    Task_Create_Task(&task1, Task_Pin_Toggler_C7, NULL, NULL, NULL, 0);
    Task_Create_Task(&task2, Task_Pin_Toggler_C8, NULL, NULL, NULL, 0);
}
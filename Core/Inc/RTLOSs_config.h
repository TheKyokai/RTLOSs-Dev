#ifndef RTLOSS_CONFIG_H
#define RTLOSS_CONFIG_H


#define config_TASK_TICK_TIMESLICE  3

#define config_CPU_CLOCK_HZ         8000000UL
#define config_TICK_HZ              1000UL


// Lowest number is the one with the highest priority
#define config_MAX_TASK_PRIORITY    15  // Should not be changed below 2 and above 32
#define config_TASK_PRIORITY_COUNT  ( config_MAX_TASK_PRIORITY + 1 )
#define config_TIMER_TASK_PRIORITY  1   // Designed to allow for timer starvation - Not advised
#define config_IDLE_TASK_PRIORITY   ( config_MAX_TASK_PRIORITY - 2 )

#define config_DEFAULT_STACK_SIZE   ( 1 << 8 )

#endif
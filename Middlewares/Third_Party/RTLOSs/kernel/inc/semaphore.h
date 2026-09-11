#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "stddef.h"
#include "stdint.h"
#include "list.h"
#include "enum_defs.h"

/*
    API     - Semaphore_*
    SYSCALL - SEM_*
*/

#define SEMAPHORE_FIFO 0
#define SEMAPHORE_PRIORITY 1

typedef struct Semaphore Semaphore;

struct Semaphore
{
    uint32_t val;
    List sem_queue;
    int priority;
};

typedef Semaphore* Semaphore_t;

void SEM_Init();
void SEM_Tick_Update();

// Part of the API
int Semaphore_Create(Semaphore_t *handle, uint32_t initial_val, int priority);
SEM_STATUS Semaphore_Acquire(Semaphore_t semaphore, uint32_t timeout);
void Semaphore_Release(Semaphore_t semaphore);
int Semaphore_Delete(Semaphore_t semaphore);
uint32_t Semaphore_Get_Value(Semaphore_t semaphore);   // No SysCall counterpart - plain atomic read, racy by nature (snapshot only)


// Used by SysCall

int SEM_Create(Semaphore_t *handle, uint32_t initial_val, int priority);
SEM_STATUS SEM_Acquire(Semaphore_t semaphore, uint32_t timeout);
void SEM_Release(Semaphore_t semaphore);
int SEM_Delete(Semaphore_t semaphore);


// Syscall arg structs

struct SEM_Create_args
{
        Semaphore_t* handle;
        uint32_t initial_val;
        int priority;
};

struct SEM_Acquire_args
{
        Semaphore_t semaphore;
        uint32_t timeout;
};

struct SEM_Release_args
{
        Semaphore_t semaphore;
};

struct SEM_Delete_args
{
        Semaphore_t semaphore;
};


#endif
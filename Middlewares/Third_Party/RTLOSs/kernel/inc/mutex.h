#ifndef MUTEX_H
#define MUTEX_H

#include "stddef.h"
#include "stdint.h"
#include "enum_defs.h"
#include "list.h"

/*
    API     - Mutex_*
    SYSCALL - MTX_*
*/

#define MUTEX_FIFO 0
#define MUTEX_PRIORITY 1

typedef struct TCB TCB;

typedef struct Mutex Mutex;

struct Mutex 
{
    uint32_t val;
    List mutex_queue;
    TCB* holder;
    int priority;
};

typedef Mutex* Mutex_t;

void MTX_Init();
void MTX_Tick_Update();

// Part of the API
int Mutex_Create(Mutex_t *handle, uint32_t initial_val, int priority);
SEM_STATUS Mutex_Acquire(Mutex_t mutex, uint32_t timeout);
void Mutex_Release(Mutex_t mutex);
int Mutex_Delete(Mutex_t mutex);   // Does not check holder - releasing/holding discipline is the caller's responsibility


// Used by SysCall

int MTX_Create(Mutex_t *handle, uint32_t initial_val, int priority);
SEM_STATUS MTX_Acquire(Mutex_t mutex, uint32_t timeout);
void MTX_Release(Mutex_t mutex);
int MTX_Delete(Mutex_t mutex);


// Syscall arg structs

struct MTX_Create_args
{
        Mutex_t* handle;
        uint32_t initial_val;
        int priority;
};

struct MTX_Acquire_args
{
        Mutex_t mutex;
        uint32_t timeout;
};

struct MTX_Release_args
{
        Mutex_t mutex;
};

struct MTX_Delete_args
{
        Mutex_t mutex;
};


#endif
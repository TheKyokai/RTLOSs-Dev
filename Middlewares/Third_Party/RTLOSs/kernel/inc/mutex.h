#ifndef MUTEX_H
#define MUTEX_H

#include "stddef.h"
#include "stdint.h"
#include "enum_defs.h"
#include "list.h"

typedef struct TCB TCB;

typedef struct Mutex Mutex;

struct Mutex 
{
    uint32_t val;
    List mutex_queue;
    TCB* holder;
};

typedef Mutex* Mutex_t;

void Mutex_Init();

int Mutex_Create(Mutex_t *handle, uint32_t initial_val);

SEM_STATUS Mutex_Acquire(Mutex_t mutex, uint32_t timeout);
void Mutex_Release(Mutex_t mutex);

void Mutex_Tick_Update();

#endif
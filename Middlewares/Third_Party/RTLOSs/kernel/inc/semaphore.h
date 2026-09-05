#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "stddef.h"
#include "stdint.h"
#include "list.h"
#include "enum_defs.h"


typedef struct Semaphore Semaphore;

struct Semaphore 
{
    uint32_t val;
    List sem_queue;
};

typedef Semaphore* Semaphore_t;

void Semaphore_Init();

int Semaphore_Create(Semaphore_t *handle, uint32_t initial_val);

SEM_STATUS Semaphore_Acquire(Semaphore_t semaphore, uint32_t timeout);
void Semaphore_Release(Semaphore_t semaphore);

void Semaphore_Tick_Update();

#endif
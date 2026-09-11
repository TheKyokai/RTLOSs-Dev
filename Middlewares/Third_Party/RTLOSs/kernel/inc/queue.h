#ifndef QUEUE_H
#define QUEUE_H

#include "stdint.h"
#include "semaphore.h"


typedef struct Queue Queue;
typedef Queue* Queue_t;

struct Queue
{
    size_t queue_size;
    size_t object_size;
    uint32_t head, tail;
    void *objects;
    Semaphore_t space_available;
    Semaphore_t item_available;
    Semaphore_t mutex;
};


int Queue_Create(Queue_t* handle, size_t queue_size, size_t object_size);

int Queue_Put(Queue_t queue, void* buffer, uint32_t timeout);
int Queue_Get(Queue_t queue, void* buffer, uint32_t timeout);

int Queue_Empty(Queue_t queue);   // Snapshot only - racy by nature, same as List_Empty

int Queue_Delete(Queue_t queue);






#endif
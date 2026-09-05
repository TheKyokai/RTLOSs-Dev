#include "heap.h"



static uint8_t heap[HEAP_SIZE];
static uint8_t free_block_status[HEAP_BLOCK_COUNT];



void Port_Heap_Init()
{
    for (int i = 0; i < HEAP_BLOCK_COUNT; i++)
        free_block_status[i] = 1;
}


void* Port_Alloc()
{
    for (int i = 0; i < HEAP_BLOCK_COUNT; i++)
        if (free_block_status[i])
        {
            free_block_status[i] = 0;
            return heap + (HEAP_BLOCK_SIZE * i);
        }
    return NULL;
}

void Port_Free(void* block)
{   
    if (block == NULL)
        return;

    uintptr_t offset = (uintptr_t) block - (uintptr_t) heap;
    
    if (offset >= HEAP_SIZE)
        return;
    if (offset % HEAP_BLOCK_SIZE != 0)
        return;

    uint32_t block_num = offset / HEAP_BLOCK_SIZE;
    
    if (block_num >= HEAP_BLOCK_COUNT)
        return;

    free_block_status[block_num] = 1;
}
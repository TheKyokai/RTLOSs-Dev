#ifndef PORT_HEAP_H
#define PORT_HEAP_H

#include "stdint.h"
#include "stddef.h"

#define HEAP_BLOCK_SIZE ( 1 << 9 )
#define HEAP_BLOCK_COUNT 10
#define HEAP_SIZE ( HEAP_BLOCK_SIZE * HEAP_BLOCK_COUNT )


void Port_Heap_Init();
void* Port_Alloc();
void Port_Free(void* block);

#endif

#ifndef PORT_HEAP_H
#define PORT_HEAP_H

#include "stdint.h"
#include "stddef.h"


#define HEAP_POOL_SIZE          ( 1 << 12 )

#define HEAP_SLI                3                       // second-level index bits -> 8 sub-buckets per FL class
#define HEAP_SL_COUNT           ( 1 << HEAP_SLI )     
#define HEAP_FL_MIN             HEAP_SLI                // smallest fl we ever compute 
#define HEAP_FL_MAX             12
#define HEAP_FL_COUNT           ( HEAP_FL_MAX - HEAP_FL_MIN + 1 )

#define HEAP_ALIGN              8                       // payload alignment
#define HEAP_MIN_BLOCK_SIZE     ( 2 * sizeof(void*) )   // must hold next_free/prev_free when block is free

#define HEAP_FREE_FLAG          ( 1 )          
#define HEAP_LAST_FLAG          ( 2 )

typedef struct TLSF_Header TLSF_Header;
typedef struct TLSF_FBHE TLSF_FBHE;


struct TLSF_Header 
{
    uint32_t size;                  // Size[31:2] | T[1](Last physical block flag) | F[0](Free flag)
    TLSF_Header* prev_phys_block;   // Bondary tag
};

struct TLSF_FBHE    // Free block header extension
{
    TLSF_Header *prev, *next;
};



void Port_Heap_Init();
void* Port_Alloc(size_t size);
void Port_Free(void* block);

#endif

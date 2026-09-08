#include "heap.h"
#include "port.h"

#define HEAP_TRUE_SIZE(size)    ((size) & ~((uint32_t) (HEAP_FREE_FLAG | HEAP_LAST_FLAG)))

static uint32_t fl_bitmap = 0;                              // First-level bitmap
static uint32_t sl_bitmap[HEAP_FL_COUNT];                   // Second-level bitmap
static TLSF_Header* free_block_map[HEAP_FL_COUNT][HEAP_SL_COUNT];

static uint8_t heap[HEAP_POOL_SIZE];                        // Memory pool / heap

// Floor mapping
static inline void map_insert(size_t size, int* f_out, int* s_out)
{
    // Calculating f and s
    int f = 31 - __builtin_clz(size);
    if (f < HEAP_FL_MIN) f = HEAP_FL_MIN;
    int s = (size - (1U << f)) / (1U << (f - HEAP_SLI));
    *f_out = f - HEAP_FL_MIN;
    *s_out = s;
}

// Round up mapping
static inline void map_find(size_t size, int* f_out, int* s_out)
{
    // Calculating f and s
    int f = 31 - __builtin_clz(size);
    if (f >= HEAP_FL_MIN)
        size += (1U << (f - HEAP_SLI)) - 1;     // Rounding up to the next sub-bucket
    map_insert(size, f_out, s_out);
}


void Port_Heap_Init()
{
    
    size_t usable = sizeof(heap) - sizeof(TLSF_Header);
    int f, s;
    map_insert(usable, &f, &s);

    // Bitmap initialization
    for (int i=0; i<HEAP_FL_COUNT; i++)
        sl_bitmap[i] = 0;

    fl_bitmap = (1U << f);
    sl_bitmap[f] = (1U << s);
    
    // free_block_map initialization
    TLSF_Header* biggest_header = free_block_map[f][s] = (TLSF_Header*) heap;
    biggest_header->size = usable | HEAP_LAST_FLAG | HEAP_FREE_FLAG;
    biggest_header->prev_phys_block = NULL;
    
    TLSF_FBHE* extension = (TLSF_FBHE*) (biggest_header + 1);
    extension->prev = NULL;
    extension->next = NULL;
}


static inline int fallback_block_search_and_split(size_t size, int* f, int* s)
{
    // Second-level check
    uint32_t sl_map = sl_bitmap[*f] & (~0U << *s);
    if (sl_map)
        *s = __builtin_ctz(sl_map);
    else 
    {
        // First-level check
        uint32_t fl_map = fl_bitmap & (~0U << *f);
        if (fl_map)
        {
            *f = __builtin_ctz(fl_map);
            *s = __builtin_ctz(sl_bitmap[*f]);
        }
        else 
            return 1;   // Not enough free memory
    }

    uint32_t leftover = HEAP_TRUE_SIZE(free_block_map[*f][*s]->size) - size;
    if (leftover >= sizeof(TLSF_Header) + HEAP_MIN_BLOCK_SIZE)
    {
        // Split block
        TLSF_Header* original_block = free_block_map[*f][*s];
        TLSF_FBHE* original_extension = (TLSF_FBHE*) (original_block + 1);

        // Remove original block
        free_block_map[*f][*s] = original_extension->next;
        if (original_extension->next)
        {
            TLSF_FBHE* next_extension = (TLSF_FBHE*) (original_extension->next + 1);
            next_extension->prev = NULL;
        }

        original_extension->next = NULL;
        original_extension->prev = NULL;

        // Bitmap clearing
        if (!free_block_map[*f][*s])
        {
            sl_bitmap[*f] &= ~(1U << *s);
            if (!sl_bitmap[*f])
                fl_bitmap &= ~(1U << *f);
        }


        // Split original block
        TLSF_Header* new_block = (TLSF_Header*) (((uint8_t*) original_block) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(original_block->size));
        TLSF_FBHE* new_extension = (TLSF_FBHE*) (new_block + 1);

        new_block->size = (leftover - sizeof(TLSF_Header)) | HEAP_FREE_FLAG | (original_block->size & HEAP_LAST_FLAG);
        original_block->size = size | HEAP_FREE_FLAG;

        new_block->prev_phys_block = original_block;

        // Fix up the block physically after new_block, if any, so its backward link is correct
        if (!(new_block->size & HEAP_LAST_FLAG))
        {
            TLSF_Header* following_block = (TLSF_Header*) (((uint8_t*) new_block) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(new_block->size));
            following_block->prev_phys_block = new_block;
        }

        // Insert new blocks
        int new_f, new_s;
        map_insert(HEAP_TRUE_SIZE(new_block->size), &new_f, &new_s);
        if (free_block_map[new_f][new_s])
            ((TLSF_FBHE*) (free_block_map[new_f][new_s] + 1))->prev = new_block;
        new_extension->next = free_block_map[new_f][new_s];
        new_extension->prev = NULL;
        free_block_map[new_f][new_s] = new_block;

        map_insert(HEAP_TRUE_SIZE(original_block->size), f, s);
        if (free_block_map[*f][*s])
            ((TLSF_FBHE*) (free_block_map[*f][*s] + 1))->prev = original_block;
        original_extension->next = free_block_map[*f][*s];
        original_extension->prev = NULL;
        free_block_map[*f][*s] = original_block;

        fl_bitmap |= (1U << new_f) | (1U << *f);
        sl_bitmap[new_f] |= (1U << new_s);
        sl_bitmap[*f] |= (1U << *s);
    }

    return 0;
}

void* Port_Alloc(size_t size)
{
    if (!size)  return NULL;
    Port_Disable_Interrupts();

    // Size alignment update
    size = (size + (HEAP_ALIGN - 1)) & ~(size_t)(HEAP_ALIGN - 1);
    if (size < HEAP_MIN_BLOCK_SIZE) size = HEAP_MIN_BLOCK_SIZE;
    
    int f, s;
    map_find(size, &f, &s);
    TLSF_Header* selected_block = NULL;
    TLSF_FBHE* sb_extension = NULL;

    if (!((fl_bitmap & (1U << f)) && (sl_bitmap[f] & (1U << s))))
        // Fallback search and block splitting since there are no free blocks with the approximate required size
        if (fallback_block_search_and_split(size, &f, &s))
        {
            // No block found
            Port_Enable_Interrupts();
            return NULL;    
        }
    
    // Adjusting the selected block metadata
    selected_block = free_block_map[f][s];
    sb_extension = (TLSF_FBHE*) (selected_block + 1);

    free_block_map[f][s] = sb_extension->next;
    if (sb_extension->next)
    {
        TLSF_FBHE* next_extension = (TLSF_FBHE*) (sb_extension->next + 1);
        next_extension->prev = NULL;
    }

    sb_extension->next = NULL;
    sb_extension->prev = NULL;
    selected_block->size &= ~HEAP_FREE_FLAG;


    // Bitmap clearing
    if (!free_block_map[f][s])
    {
        sl_bitmap[f] &= ~(1U << s);
        if (!sl_bitmap[f])
            fl_bitmap &= ~(1U << f);
    }

    Port_Enable_Interrupts();
    return (selected_block? (void*) (selected_block + 1) : NULL);
}

void Port_Free(void* block)
{   
    
}
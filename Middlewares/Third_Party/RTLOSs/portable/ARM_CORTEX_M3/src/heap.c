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


static inline void remove_free_block(TLSF_Header* block)
{
    int f, s;
    map_insert(HEAP_TRUE_SIZE(block->size), &f, &s);
    TLSF_FBHE* ext = (TLSF_FBHE*) (block + 1);

    if (ext->prev)
        ((TLSF_FBHE*) (ext->prev + 1))->next = ext->next;
    else
        free_block_map[f][s] = ext->next;      // block was the head

    if (ext->next)
        ((TLSF_FBHE*) (ext->next + 1))->prev = ext->prev;

    if (!free_block_map[f][s])
    {
        sl_bitmap[f] &= ~(1U << s);
        if (!sl_bitmap[f])
            fl_bitmap &= ~(1U << f);
    }
}


static inline void insert_free_block(TLSF_Header* block)
{
    int f, s;
    map_insert(HEAP_TRUE_SIZE(block->size), &f, &s);
    TLSF_FBHE* ext = (TLSF_FBHE*) (block + 1);

    ext->prev = NULL;
    ext->next = free_block_map[f][s];
    if (free_block_map[f][s])
        ((TLSF_FBHE*) (free_block_map[f][s] + 1))->prev = block;
    free_block_map[f][s] = block;

    fl_bitmap |= (1U << f);
    sl_bitmap[f] |= (1U << s);
}



void Heap_Init()
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
        uint32_t fl_map = fl_bitmap & (~0U << (*f + 1));
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
        remove_free_block(original_block);

        // Split original block
        TLSF_Header* new_block = (TLSF_Header*) (((uint8_t*) original_block) + sizeof(TLSF_Header) + size);

        new_block->size = (leftover - sizeof(TLSF_Header)) | HEAP_FREE_FLAG | (original_block->size & HEAP_LAST_FLAG);
        original_block->size = size | HEAP_FREE_FLAG;

        new_block->prev_phys_block = original_block;

        // Fix up the block physically after new_block, if any, so its backward link is correct
        if (!(new_block->size & HEAP_LAST_FLAG))
        {
            TLSF_Header* following_block = (TLSF_Header*) (((uint8_t*) new_block) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(new_block->size));
            following_block->prev_phys_block = new_block;
        }

        insert_free_block(new_block);
        insert_free_block(original_block);

        // Report back where original_block ended up so the caller can pop it
        map_insert(HEAP_TRUE_SIZE(original_block->size), f, s);
    }

    return 0;
}

void* Heap_Alloc(size_t size)
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



// Merges two blocks if they are both free and physical neighbours
static inline void merge_blocks(TLSF_Header* first, TLSF_Header* second)
{
    if (!first || !second || first == second) return;

    if ((first->size & HEAP_FREE_FLAG) && (second->size & HEAP_FREE_FLAG) 
        && (first == second->prev_phys_block || first->prev_phys_block == second))
    {
        remove_free_block(first);
        remove_free_block(second);
        TLSF_Header *kept, *merged;
        if (first == second->prev_phys_block)
        {
            kept = first;
            merged = second;
        }
        else
        {
            kept = second;
            merged = first;
        }


        // Fix up physical block after merged if it exists
        if (!(merged->size & HEAP_LAST_FLAG))
        {
            TLSF_Header* after = (TLSF_Header*) (((uint8_t*) merged) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(merged->size));
            after->prev_phys_block = kept;
        }

        kept->size = (HEAP_TRUE_SIZE(kept->size) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(merged->size)) | HEAP_FREE_FLAG | (merged->size & HEAP_LAST_FLAG);

        insert_free_block(kept);
    }
}

void Heap_Free(void* block)
{   
    if (!block) return;
    Port_Disable_Interrupts();
    
    TLSF_Header* freed_block = ((TLSF_Header*) block) - 1;
    freed_block->size |= HEAP_FREE_FLAG;
    insert_free_block(freed_block);

    TLSF_Header* next = (TLSF_Header*)(((uint8_t*)freed_block) + sizeof(TLSF_Header) + HEAP_TRUE_SIZE(freed_block->size));
    TLSF_Header* prev = freed_block->prev_phys_block;
    
    if (!(freed_block->size & HEAP_LAST_FLAG))
        merge_blocks(freed_block, next);
    merge_blocks(prev, freed_block);
    
    Port_Enable_Interrupts();
}

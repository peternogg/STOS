//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>
#include <string.h>

#include <sysio.h>
const void* NULL = (void *)0;
//#include "sysio.h"

#include "mymalloc.h"

#define MIN_BLOCK_SIZE  16

#define BUSY_BIT  0x40000000
#define HASH_VAL  0x3BADEBAD
#define HASH_MASK 0x3FFFFFFF

typedef struct header_s
{
    int hash;
    int size;
    struct header_s *next;
    struct header_s *prev;
} header_t;

typedef struct
{
    int hash;
    int size;
} busy_header_t;

//#define CLR_BUSY(a) {((a)->hash) = HASH_VAL;}
//#define SET_BUSY(a) {((a)->hash) =  (BUSY_BIT | HASH_VAL);}
//#define IS_BUSY(a)  (((a)->hash) & (BUSY_BIT))
//
//#define BUSY_HEADER(a) ((header_t *)((char *)(a)- sizeof(busy_header_t)))

void CLR_BUSY(header_t *head) { head->hash = HASH_VAL; }
void SET_BUSY(header_t *head) { head->hash = HASH_VAL | BUSY_BIT; }
int  IS_BUSY(header_t *head)  { return head->hash & BUSY_BIT; }
header_t *BUSY_HEADER(void *head) { return (header_t*)(head - sizeof(busy_header_t)); }


char *g_Memory;
int g_Mem_Size;
header_t *Free_List;
header_t *Next_Free;

//************************************
void set_busy_headers(void *addr, header_t *header)
{
    SET_BUSY(header);
    memcpy(addr, header, sizeof(busy_header_t));
}
//************************************
void set_free_headers(void *addr, header_t *header)
{
    CLR_BUSY(header);
    memcpy(addr, header, sizeof(header_t));
    memcpy((char*)addr + header->size - sizeof(header_t), header, 
            sizeof(header_t));
}
//************************************
void my_mem_init(void *ptr, int size)
{
    header_t header;

    g_Memory = (char *)ptr;
    g_Mem_Size = size;

    Free_List = (header_t *)g_Memory;
    Next_Free = Free_List;

    header.hash = HASH_VAL;
    header.next = (header_t*)NULL;
    header.prev = (header_t*)NULL;
    header.size = g_Mem_Size;

    set_free_headers(g_Memory, &header);
}
//************************************
int valid_size(int size)
{
    // make sure the block is big enough to make a free block out of it
    if (size < 2*sizeof(header_t) - sizeof(busy_header_t)) 
    {
        size = 2*sizeof(header_t) - sizeof(busy_header_t); 
    }

    // round size up to multiple of 8
    if (size & 0x07) size = (size + 8) & ~0x07;

    return size;
}
//************************************
void *my_malloc(int size)
{
    header_t *start_ptr = Next_Free;
    header_t *free;
    header_t *busy;

    if (start_ptr == NULL) start_ptr = Free_List;

    // are we out of memory?
    if (start_ptr == NULL) return NULL;
    
    size = valid_size(size);

    free = start_ptr;

    do
    {
        // Found a big enough block?
        if (free->size >= size + sizeof(busy_header_t))
        {
            // Big enough to split?
            if (free->size > 
                    (size +  sizeof(busy_header_t) +
                     2*sizeof(header_t) )
               )
            {
                // split
                int new_size = free->size - size - sizeof(busy_header_t);

                busy = (header_t *)(((char *)free) + new_size);

                free->size = new_size;
                set_free_headers((char *)free, free);

                busy->size = size + sizeof(busy_header_t);
                set_busy_headers((char *)busy, busy);
                return (void *)((char *)busy + sizeof(busy_header_t));
            }
            else
            {
                // Simply remove this block from the free list
                if (free->prev != NULL)
                    free->prev->next = free->next;
                else
                    Free_List = free->next;

                if (free->next != NULL)
                    free->next->prev = free->prev;

                Next_Free = free->next;

                // set busy bit and return address
                SET_BUSY(free);
                return ((char *)free) + sizeof(busy_header_t);
            }
        }

        free = free->next;
        if (free == NULL) free = Free_List;
    } while (free != start_ptr);

    return NULL;
}

//********************************************
// Find the block before the current one
header_t *prev_block(header_t *block)
{
    // trailing header
    header_t *prev;
    int size;

    prev = (header_t*)((char*)block - sizeof(header_t));
    // make sure we didn't go before the beginning of memory
    if ( prev < (header_t*)g_Memory) return NULL;

    // this only works for free blocks, so require busy bit to be zero
    // (in other words, don't mask off the busy bit)
    if (prev->hash != HASH_VAL) return NULL;
    size = prev->size;
    prev = (header_t*)((char *)prev - size + sizeof(header_t));

    // validate that the header we found at the beginning matches the one
    // at the end of the block
    if (prev->hash != HASH_VAL) return NULL;
    if (prev->size != size) return NULL;

    return prev;
}
//********************************************
// Find the block after the current one
header_t *next_block(header_t *block)
{
    // get the header
    header_t *next = (header_t*)((char *)block + block->size);

    // Make sure we didn't go past the end of memory
    if ( (char *)next >= (char *)&g_Memory[g_Mem_Size]) return NULL;

    // this only works for free blocks, so require busy bit to be zero
    if (next->hash != HASH_VAL) return NULL;

    // should probably check that we have a valid trailing header if the 
    // block is free, but we'll skip that
    return next;
}
//**********************************************
void my_free(void *ptr) 
{
    // Get address of my header and blocks on either side
    header_t *block = BUSY_HEADER(ptr);
    header_t *prev = prev_block(block);
    header_t *next = next_block(block);

    // if prev, next aren't free blocks, pointers will be NULL
    if (prev!=NULL && !IS_BUSY(prev) && 
        next != NULL && !IS_BUSY(next))
    {
        // coallesce all three

        // next is going away, make sure Free_List and Next_Free 
        // don't point there
        if (Free_List == next) Free_List = next->next;
        if (Next_Free == next) Next_Free = next->next;

        // Unlink next
        if (next->next != NULL) next->next->prev = next->prev;
        if (next->prev != NULL) next->prev->next = next->next;
        
        // new size of block
        prev->size += block->size + next->size;
        set_free_headers(next, next);
    } 
    else if (prev != NULL && !IS_BUSY(prev) && 
              (next == NULL || IS_BUSY(next)))
    {
        // coallesce with prev
        // Don't need to update free list, just size of block.
        prev->size += block->size;
        set_free_headers(prev, prev);
    } else if ((prev==NULL || IS_BUSY(prev) ) && 
            next != NULL && !IS_BUSY(next))
    {
        // coallesce with next
        // 
        // first update the free list so it points to block 
        // instead of next
        if (next->prev != NULL) next->prev->next = block;
        if (next->next != NULL) next->next->prev = block;

        // Fix Free_List and Next_Free if necessary
        if (Free_List == next) Free_List = block;
        if (Next_Free == next) Next_Free = block;

        // update size and pointers in block's header
        block->size += next->size;
        block->next = next->next;
        block->prev = next->prev;
        set_free_headers(block, block);
    }
    else
    {
        // don't coallesce, add to beginning of Free_List
        block->prev = (header_t*)NULL;
        block->next = Free_List;
        if (Free_List != NULL) Free_List->prev = block;
        set_free_headers(block, block);
        Free_List = block;
    }
}
//**********************************************
void *my_get_largest(int *size)
{
    header_t *start_ptr = Next_Free;
    header_t *free;
    header_t *max_free = NULL;

    if (size != NULL) *size = 0;

    if (start_ptr == NULL) start_ptr = Free_List;

    // are we out of memory?
    if (start_ptr == NULL) return NULL;
    
    free = start_ptr;
    max_free = free;

    do
    {
        if (free->size > max_free->size) max_free = free;

        free = free->next;
        if (free == NULL) free = Free_List;
    } while (free != start_ptr);

    // remove this block from the free list
    if (max_free->prev != NULL)
        max_free->prev->next = max_free->next;
    else
        Free_List = max_free->next;

    if (max_free->next != NULL)
        max_free->next->prev = max_free->prev;

    Next_Free = max_free->next;

    // set busy bit and return address
    SET_BUSY(max_free);
    if (size != NULL) *size = max_free->size;
    return ((char *)max_free) + sizeof(busy_header_t);

    return NULL;
}
//**********************************************
void *my_set_limit(void *base, void *limit)
{
    int size;
    int new_size;

    // can't have negative size blocks
    if (limit < base) return NULL;

    header_t *busy_block = BUSY_HEADER(base);
    header_t *free_block;

    size = limit - base + sizeof(busy_header_t);

    size = valid_size(size);

    // memory overflow error
    if (size > busy_block->size) return NULL;

    // using the whole block
    if (size + MIN_BLOCK_SIZE >= busy_block->size) return base;

    // left-over space becomes a new free block
    new_size = busy_block->size - size;

    busy_block->size = size;

    free_block = (header_t *)((char *)busy_block + size);

    free_block->size = new_size;
    CLR_BUSY(free_block);
    free_block->prev = (header_t*)NULL;
    free_block->next = Free_List;
    if (Free_List != NULL) Free_List->prev = free_block;
    set_free_headers(free_block, free_block);
    Free_List = free_block;

    return base;
}
//**********************************************
int my_validate()
{
    char *addr = g_Memory;
    char *stop_addr = g_Memory + g_Mem_Size;
    header_t *block;

    while (addr < stop_addr)
    {
        block = (header_t *)addr;
        if ((block->hash & HASH_MASK) != HASH_VAL)
        {
            prints("Invalid hash at addr ");
            printxn((char *)block - g_Memory, 4);
            prints("\n");
            return 0;
        }

        if (block->size <= 0 || block->size > g_Mem_Size)
        {
            prints("Invalid size at addr ");
            printxn((char *)block - g_Memory, 4);
            prints("\n");
            return 0;
        }

        addr = addr + block->size;
    }

    return 1;
}
//**********************************************
void my_print_mem()
{
    char *addr = g_Memory;
    char *stop_addr = g_Memory + g_Mem_Size;
    header_t *block;

    // if memory is bad, we won't try to print it
    if (!my_validate()) return;

    prints("Free List: 0x");
    if (Free_List == NULL)
        prints("0000");
    else
        printxn((char *)Free_List - g_Memory, 4);
    prints("\nNext Free: 0x");
    if (Next_Free == NULL)
        prints("0000");
    else
        printxn((char *)Next_Free - g_Memory, 4);
    prints("\n");

    while (addr < stop_addr)
    {
        block = (header_t *)addr;
        if (IS_BUSY(block))
        {
            prints("busy: 0x");
            printxn(((char *)block) - g_Memory, 4);
            prints(" 0x");
            printxn(block->size, 4);
            prints(" 0x");
            printxn(block->hash, 4);
            prints("\n");
            //fprintf(stderr, "busy: 0x%04lX 0x%04X 0x%08X\n", 
            //        ((char *)block) - g_Memory, block->size, block->hash);
        }
        else
        {
            prints("free: 0x");
            printxn(((char *)block) - g_Memory, 4);
            prints(" 0x");
            printxn(block->size, 4);
            prints(" 0x");
            printxn(block->hash, 4);
            prints(": ");
            //fprintf(stderr, "free: 0x%04lX 0x%04X 0x%08X: ", 
            //        (char *)block - g_Memory, block->size, block->hash);

            if (block->next == NULL)
            {
                prints("0x---- ");
            }
            else
            {
                prints("0x");
                printxn((char *)(block->next) - g_Memory, 4);
                prints(" ");
            }

            if (block->prev == NULL)
            {
                prints("0x---- ");
            }
            else
            {
                prints("0x");
                printxn((char *)(block->prev) - g_Memory, 4);
                prints(" ");
            }

            prints("\n");
        }

        addr = addr + block->size;
    }
}

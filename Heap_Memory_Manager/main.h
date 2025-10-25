#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

// Heap configuration constants
#define HEAP_MAX_SIZE 10000
#define MIN_BLOCK_SIZE 8
#define HEADER_SIZE sizeof(BlockHeader)


// Block header structure for memory management
typedef struct BlockHeader {
    bool is_allocated;              // Allocation status flag 1 bytes
    uint32_t size;                    // Size of usable memory (excluding header) 4 bytes
    struct BlockHeader *prev_free;  // Pointer to previous free block in free list 8 bytes 
    struct BlockHeader *next_free;  // Pointer to next free block in free list 8 bytes 
} BlockHeader;





// Function declarations
void HmmInit(void);
void *HmmAlloc(uint32_t needed_size); // we got the size in bytes 
void HmmFree(void *ptr);
void Coalescing_blocks(BlockHeader *head);
void Heap_Shrinking(void);
int ceiling(int a , int b);

#endif // MAIN_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Global variable definitions
uint8_t heap[HEAP_MAX_SIZE];
size_t program_break;
BlockHeader *free_list_head;

int ceiling(int a , int b)
{
    return (a + b - 1) / b; // round up to the nearest multiple of b
}



void Coalescing_blocks(BlockHeader *head){
    if (head == NULL) {
        return;
    }
    
    BlockHeader *cur = head;

    while(cur->next_free != NULL){
        BlockHeader * next_add = (BlockHeader *)((uint8_t*)cur + HEADER_SIZE + cur->size);

        // same address this means that they are consecutive free 
        if(next_add == cur->next_free){
            cur->size += HEADER_SIZE + cur->next_free->size;
            
            // Store the next free block to remove
            BlockHeader *next_free = cur->next_free;
            
            // Update pointers
            if(next_free->next_free != NULL){
                cur->next_free = next_free->next_free;
                next_free->next_free->prev_free = cur;
            } else {
                cur->next_free = NULL;
            }
        } else {
            cur = cur->next_free;
        }
    }
}

void Heap_Shrinking(void){
    // Heap shrinking logic
    // Min size to trigger the shrink
    uint32_t shrink_thershold = 400; // 400 bytes
    
    // Calculate minimum heap size (initial block: header + margin)
    uint32_t min_heap_size = HEADER_SIZE + 400;

    // Get the address of the end of the heap
    uint8_t *heap_end = heap + program_break; 

    // Check if free list is not empty
    if (free_list_head != NULL) {
        // Check the first free block (could be coalesced with freed block)
        uint8_t *block_end = (uint8_t *)free_list_head + HEADER_SIZE + free_list_head->size;

        // Check if this block at the end of heap
        if (block_end == heap_end) {
            size_t shrink_amount = HEADER_SIZE + free_list_head->size;
            
            // Only shrink if size meets threshold and won't go below minimum
            if (shrink_amount >= shrink_thershold && (program_break - shrink_amount) >= min_heap_size) {
                // Remove top block from free list
                BlockHeader *next_free = free_list_head->next_free;
                if (next_free != NULL) {
                    next_free->prev_free = NULL;
                }
                free_list_head = next_free;

                // Shrink the program break
                program_break -= shrink_amount;
            }
        }
    }
}

/**
 * Initialize the heap memory manager
 * Sets up the initial heap state with one large free block
 */
void HmmInit(void) {
    // Initialize program break to start of heap
    uint32_t margin_size , total_needed_init_space;
    program_break = 0;

    // Increase the program break to save the meta data of the Head of free list
     margin_size = 400; // make 400 bytes as margin 
     total_needed_init_space = HEADER_SIZE + margin_size;
     total_needed_init_space = ceiling(total_needed_init_space, MIN_BLOCK_SIZE) * MIN_BLOCK_SIZE;

     program_break += total_needed_init_space;

    // Add the Block_header to the start of the heap 
    BlockHeader *initial_header = (BlockHeader *)heap;
    initial_header->is_allocated = false;
    initial_header->size = total_needed_init_space - HEADER_SIZE;
    initial_header->prev_free = NULL;
    initial_header->next_free = NULL;

    free_list_head = initial_header;


    // Print initialization details
    printf("HMM Initialized (OS-like behavior with 8-byte alignment):\n");
    printf("  Heap size: %d bytes (%d blocks of 8 bytes)\n", HEAP_MAX_SIZE, HEAP_MAX_SIZE / 8);
    printf("  BlockHeader size: %zu bytes (%zu blocks of 8 bytes)\n", HEADER_SIZE, HEADER_SIZE / 8);
    printf("  Program break location : %zu \n" , program_break);
    printf("  Initial free block size: %u bytes (%u blocks)\n", initial_header->size, initial_header->size / 8);
    printf("  Free list head: %p\n", (void*)free_list_head);

}

void *HmmAlloc(uint32_t needed_size) {
    // Validate input
    if (needed_size == 0) {
        return NULL;
    }

    // Traverse the linked list of free block with first fit algorithm
    BlockHeader *cur = free_list_head;
    BlockHeader *ptr_allocated = NULL;

    uint32_t num_of_blocks = ceiling(needed_size, MIN_BLOCK_SIZE);
    uint32_t aligned_size = num_of_blocks * MIN_BLOCK_SIZE;

    uint32_t MIN_REMAINDER_SIZE = HEADER_SIZE + MIN_BLOCK_SIZE;

    while(cur != NULL){
        if(!cur->is_allocated && cur->size >= aligned_size){

            // Check if we can split
            if(cur->size > aligned_size + MIN_REMAINDER_SIZE) {
                // Create new free block from remainder
                BlockHeader * new_free_block = (BlockHeader *)((uint8_t*)cur + HEADER_SIZE + aligned_size);

                // setup the free block meta data
                new_free_block->size = cur->size - aligned_size - HEADER_SIZE; 
                new_free_block->is_allocated = false;
                new_free_block->prev_free = cur->prev_free;
                new_free_block->next_free = cur->next_free;

                if(cur->prev_free != NULL){
                    cur->prev_free->next_free = new_free_block;
                }

                if(cur->next_free != NULL){
                    cur->next_free->prev_free = new_free_block;
                }

                if(free_list_head == cur){
                    free_list_head = new_free_block;
                }

                // Mark the block as allocated
                ptr_allocated = cur;
                ptr_allocated->size = aligned_size;

            }
            else { // Very Small Size to Split So We acquire all the space
                ptr_allocated = cur;

                // Remove the block from the free list completely
                if(cur->prev_free != NULL){
                    cur->prev_free->next_free = cur->next_free;
                }

                if(cur->next_free != NULL){
                    cur->next_free->prev_free = cur->prev_free;
                }

                if(free_list_head == cur){
                    free_list_head = cur->next_free;
                }

            }

            // setup the allocated block metadata

            ptr_allocated->is_allocated = true;
            ptr_allocated->next_free = NULL;
            ptr_allocated->prev_free = NULL;

            break;
        }

        cur = cur->next_free; // procced the pointer
    }


    // No such space to allcoate
    if(ptr_allocated == NULL){

        // Calculate needed space in bytes
        size_t total_needed_space = ceiling(HEADER_SIZE + aligned_size, MIN_BLOCK_SIZE); 
        total_needed_space *= MIN_BLOCK_SIZE;

        // Check if we hit the max size of the heap
        if(program_break + total_needed_space > HEAP_MAX_SIZE){
            return NULL;
        }

        // Allocate new block
        BlockHeader * new_block = (BlockHeader *)(heap + program_break);

        // Setup metadata
        new_block->is_allocated = true;
        new_block->size = aligned_size;
        new_block->next_free = NULL;
        new_block->prev_free = NULL;

        // Increment program break
        program_break += total_needed_space;

        ptr_allocated = new_block;
        
    }

    return ((void *)(ptr_allocated + HEADER_SIZE));

}


void HmmFree(void *ptr) {
    // Validate input
    if (ptr == NULL) {
        return;
    }

    // Calculate block header address
    BlockHeader *block_to_free = (BlockHeader *)((uint8_t *)(ptr) - HEADER_SIZE);

    // Check for double-free
    if (!block_to_free->is_allocated) {
        return; // Already free
    }

    // Mark as free
    block_to_free->is_allocated = false;

    // Insert at beginning of free list
    block_to_free->next_free = free_list_head;
    block_to_free->prev_free = NULL;

    if (free_list_head != NULL) {
        free_list_head->prev_free = block_to_free;
    }

    free_list_head = block_to_free;

    // Coalescing logic - merge adjacent free blocks
    Coalescing_blocks(free_list_head);
    
    // Heap shrinking logic
    Heap_Shrinking();

}

int main(int argc, char **argv) {
    // Initialize the heap memory manager
    HmmInit();
    
    printf("\nHeap Memory Manager initialized successfully!\n");
    
    return 0;
}
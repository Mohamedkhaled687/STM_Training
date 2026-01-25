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

    return ((void *)(ptr_allocated + 1));

}

void HmmFree_test(void) {
    printf("\n========== HmmFree Test ==========\n");
    
    // Test 1: Basic Free
    printf("\n--- Test 1: Basic Free ---\n");
    void *ptr1 = HmmAlloc(16);
    printf("  Allocated 16 bytes at %p\n", ptr1);
    printf("  Program break before free: %zu\n", program_break);
    printf("  Free list head before free: %p\n", (void*)free_list_head);
    HmmFree(ptr1);
    printf("  After freeing:\n");
    printf("  Free list head: %p\n", (void*)free_list_head);
    printf("  [PASS] Basic free completed\n");
    
    // Test 2: Coalescing Blocks
    printf("\n--- Test 2: Coalescing Blocks ---\n");
    void *ptr2 = HmmAlloc(32);
    void *ptr3 = HmmAlloc(32);
    printf("  Allocated ptr2 at %p\n", ptr2);
    printf("  Allocated ptr3 at %p\n", ptr3);
    HmmFree(ptr3);
    printf("  Free list head after freeing ptr3: %p\n", (void*)free_list_head);
    printf("  Block size after freeing ptr3: %u bytes\n", free_list_head->size);
    HmmFree(ptr2);
    printf("  Free list head after freeing ptr2: %p\n", (void*)free_list_head);
    printf("  Coalesced block size: %u bytes\n", free_list_head->size);
    if (free_list_head->size > 32) {
        printf("  [PASS] Coalescing worked!\n");
    } else {
        printf("  [FAIL] Coalescing did not work.\n");
    }
    
    // Test 3: Heap Shrinking
    printf("\n--- Test 3: Heap Shrinking ---\n");
    void *ptr4 = HmmAlloc(500);
    printf("  Allocated 500 bytes at %p\n", ptr4);
    printf("  Program break before free: %zu\n", program_break);
    HmmFree(ptr4);
    printf("  Program break after free: %zu\n", program_break);
    printf("  [PASS] Heap shrinking test completed\n");
    
    printf("\n========== HmmFree Test Complete ==========\n");
}

void HmmAlloc_test(void) {
    printf("\n========== HmmAlloc Test ==========\n");
    
    // Test 1: Basic allocation
    printf("\n--- Test 1: Basic Allocation ---\n");
    void *ptr1 = HmmAlloc(16);
    if (ptr1 != NULL) {
        printf("  [PASS] Allocated 16 bytes at %p\n", ptr1);
    } else {
        printf("  [FAIL] Failed to allocate 16 bytes\n");
    }
    
    // Test 2: Another allocation
    printf("\n--- Test 2: Second Allocation ---\n");
    void *ptr2 = HmmAlloc(32);
    if (ptr2 != NULL) {
        printf("  [PASS] Allocated 32 bytes at %p\n", ptr2);
    } else {
        printf("  [FAIL] Failed to allocate 32 bytes\n");
    }
    
    // Test 3: Alignment check (1 byte should be aligned to 8 bytes)
    printf("\n--- Test 3: Alignment Test (1 byte) ---\n");
    void *ptr3 = HmmAlloc(1);
    if (ptr3 != NULL) {
        uint64_t addr = (uint64_t)ptr3;
        if (addr % 8 == 0) {
            printf("  [PASS] 1 byte allocated at %p (8-byte aligned)\n", ptr3);
        } else {
            printf("  [FAIL] 1 byte allocated at %p (NOT aligned)\n", ptr3);
        }
    } else {
        printf("  [FAIL] Failed to allocate 1 byte\n");
    }
    
    
    // Test 4: Large allocation
    printf("\n--- Test 4: Large Allocation (1000 bytes) ---\n");
    void *ptr4 = HmmAlloc(1000);
    if (ptr4 != NULL) {
        printf("  [PASS] Allocated 1000 bytes at %p\n", ptr4);
    } else {
        printf("  [FAIL] Failed to allocate 1000 bytes\n");
    }

    

    // Print final program break
    printf("\n--- Final State ---\n");
    printf("  Program break: %zu bytes\n", program_break);
    printf("  Free list head: %p\n", (void*)free_list_head);
    
    printf("\n========== HmmAlloc Test Complete ==========\n");
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

void Heap_Shrinking(void) {

    BlockHeader *current = free_list_head;

    uint32_t shrink_threshold = 400; 
    uint32_t min_heap_size = HEADER_SIZE + 400;
    uint8_t *heap_end = heap + program_break;

    if (program_break <= min_heap_size) {
        return;
    }

    // Find block at heap end
    BlockHeader *block_at_end = NULL;
    current = free_list_head;

    while (current != NULL) {
        uint8_t *block_end = (uint8_t *)current + HEADER_SIZE + current->size;
        
        if (block_end == heap_end) {
            block_at_end = current;
            break;
        }
        
        current = current->next_free;
    }

    if (block_at_end == NULL) {
        return;
    }

    size_t shrink_amount = HEADER_SIZE + block_at_end->size;
    
    if (shrink_amount >= shrink_threshold && 
        (program_break - shrink_amount) >= min_heap_size) {
        
        // Remove from free list
        if (block_at_end->prev_free != NULL) {
            block_at_end->prev_free->next_free = block_at_end->next_free;
        }
        
        if (block_at_end->next_free != NULL) {
            block_at_end->next_free->prev_free = block_at_end->prev_free;
        }
        
        if (free_list_head == block_at_end) {
            free_list_head = block_at_end->next_free;
        }

        program_break -= shrink_amount;
    }
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

    // Heap Shrinking logic
    Heap_Shrinking();

}


int main(int argc, char **argv) {
    // Initialize the heap memory manager
    HmmInit();
    
    printf("\nHeap Memory Manager initialized successfully!\n");
    
    // Run tests
    HmmFree_test();
    
    return 0;
}
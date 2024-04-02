#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "memory_allocator.h"

#ifdef USE_CANARIES
    #define CANARY_VALUE 0xcafebabe // Define the canary value
    #define CANARY_SIZE sizeof(uint32_t) // Define the size of the canary
    #define CANARY_TYPE uint32_t // Define the type of the canary
#else
    #define CANARY_SIZE 0
#endif

typedef struct Block { // Block structure to hold metadata
    size_t size; // Useable size of the block
    size_t actual_size; // Actual size of the block
    char free;  // 0=> Not free, 1=> Free
    struct Block* next; // Pointer to the next block
    struct Block* prev; // Pointer to the previous block
    #ifdef USE_CANARIES
        const char* filename; // File name where memory was allocated
        int line_number; // Line number where memory was allocated
    #endif
} __attribute__((packed)) Block; // close Block

static void* set_size_return_data_ptr(Block* block, size_t size);

static size_t MEMORY_SIZE = 0; // Total memory size
static Block* head = NULL; // Head of the memory pool
// Function to initialize the memory pool
void initialize_memory_pool(size_t size) {
    MEMORY_SIZE = size; // Set the memory size
    head = (Block*)mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (head == MAP_FAILED) { // Check if mmap failed
        perror("mmap"); // Print error message
        exit(EXIT_FAILURE); // Exit the program
    }
    // Initialize the head block
    head->size = MEMORY_SIZE - sizeof(Block) - 2 * CANARY_SIZE; // Set the size of the block
    set_size_return_data_ptr(head, head->size); // Set the data pointer
    head->actual_size = head->size; // Actual size same as useable size
    head->free = 1; // Initially, all memory is free
    head->next = NULL; // Initial block as no next block
    head->prev = NULL; // Initial block as no previous block
} // close initialize_memory_pool

// Function to destroy the memory pool
void destroy_memory_pool() {
    if (munmap(head, MEMORY_SIZE) == -1) { // Unmap the memory pool
        perror("munmap"); // Print error message
        exit(EXIT_FAILURE); // Exit the program
    }
} // close destroy_memory_pool



// Function to merge the next block if free
static inline void attempt_merge_next_block(Block* current) {
    // Merge with the next block if it is free
    if (current->next && current->next->free) { //check if next is defined and free
        current->size += sizeof(Block) + current->next->actual_size + 2 * CANARY_SIZE; // combine two sizes
        current->actual_size = current->size; // Set the actual size of the block
        current->next = current->next->next; // update next to remove middle block
        if (current->next) { // check if next is defined
            current->next->prev = current; // Update previous pointer of the next block
        } 
    }
} // close attempt_merge_next_block

// Function to merge the previous block if free
static inline void attempt_merge_prev_block(Block* current) {
    // Merge with the previous block if it is free
    if (current->prev && current->prev->free) { // check if previous is defined and free
        current->prev->size += sizeof(Block) + current->actual_size + 2 * CANARY_SIZE; 
        current->prev->actual_size = current->prev->size; // Set the actual size of the block
        current->prev->next = current->next; // Update next pointer of the previous block
        if (current->next) { // check if next is defined
            current->next->prev = current->prev; // Update previous pointer of the next block
        }
    }
} // close attempt_merge_prev_block

// Function to get the ptr for where data starts in the block
static void* set_size_return_data_ptr(Block* block, size_t size) {
    block->size = size; // Set the size of the block
    #ifdef USE_CANARIES
        CANARY_TYPE* canary;
        canary = (CANARY_TYPE*)((uint8_t*)block + sizeof(Block));
        *canary = CANARY_VALUE;
        canary = (CANARY_TYPE*)((uint8_t*)canary + CANARY_SIZE + size);
        *canary = CANARY_VALUE;
        return (void*)((uint8_t*)block + sizeof(Block) + CANARY_SIZE);
    #else
        return (void*)((uint8_t*)block + sizeof(Block));
    #endif
} // close get_data_pointer

// Function to allocate memory from the pool
#ifdef USE_CANARIES
void* mymalloc(size_t size, const char* file, int line)
#else
void* mymalloc(size_t size)
#endif
{
    Block *current = head; // Start from the head
    while (current < head + MEMORY_SIZE) { // Iterate through the blocks
        if (current->free && current->actual_size >= size) { //check if block is free and big enough
            if (current->actual_size > size + sizeof(Block) + 2 * CANARY_SIZE) { // Check if block can be split
                Block *new_block = (Block*)((uint8_t*)current + sizeof(Block) + size + 2 * CANARY_SIZE); // Create a new block and account for metadata block size
                size_t new_block_size = current->actual_size - size - sizeof(Block) - 2 * CANARY_SIZE;
                set_size_return_data_ptr(new_block, new_block_size); // Set the size of the new block accounting for metadata block size                
                new_block->actual_size = new_block_size; // Set the actual size of the new block
                new_block->free = 1; // Mark as free
                new_block->next = current->next; // set pointer for new middle block
                new_block->prev = current; // Set previous pointer for the new block
                if (current->next) { // check if next is defined
                    current->next->prev = new_block; // Update previous pointer of the next block
                }
                current->next = new_block; // next is new middle block
                current->size = size; // Set the size of the current block
                current->actual_size = size; // Block was resized to be exact
            } // close if block can be split
            current->free = 0; // mark as not free
            #ifdef USE_CANARIES
                current->filename = file; // Set the file name
                current->line_number = line; // Set the line number
            #endif
            return set_size_return_data_ptr(current, size); // Return the pointer to the user
        } // close if block is free and big enough
        current = current->next; // Move to the next block in while search
    } // close while loop
    return NULL; // Return NULL if no memory is available
} // close mymalloc

#ifdef USE_CANARIES
void check_canaries(Block* block) {
        printf("Checking canaries\n");
        CANARY_TYPE* canary;
        canary = (CANARY_TYPE*)((uint8_t*)block + sizeof(Block));
        if (*canary != CANARY_VALUE) {
            printf("Memory Corruption Detected! Allocated in file %s at line %d\n", block->filename, block->line_number);
            exit(EXIT_FAILURE);
        }
        printf("Checking canaries\n");
        canary = (CANARY_TYPE*)((uint8_t*)canary + CANARY_SIZE + block->size);
        if (*canary != CANARY_VALUE) {
            printf("Memory Corruption Detected! Allocated in file %s at line %d\n", block->filename, block->line_number);
            exit(EXIT_FAILURE);
        }
}
#endif

// Function to free memory
void myfree(void* ptr) {
    Block* current = (Block*)((uint8_t*)ptr - sizeof(Block)); // Get the metadata block from the pointer
    #ifdef USE_CANARIES
        check_canaries(current);
    #endif
    current->free = 1; // Mark as free
    // current->size = current->actual_size; // Complete block is free 
    attempt_merge_next_block(current); // Attempt to merge with the next block
    attempt_merge_prev_block(current);  // Attempt to merge with the previous block
} // close myfree

// Function to display the blocks
// This function is only used for debugging purposes and should not be used in production
void display_blocks() {
    Block *current = head; // Start from the head
    int i = 0; // Counter for the block number
    while (current) { // Iterate through the blocks
        printf("Block %d: [Address: %p, Size: %lu, Free: %s]\n", ++i, (void*)current, current->size, current->free ? "true" : "false");
        current = current->next; // Move to the next block
    } // close while loop
} // close display_blocks
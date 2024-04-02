#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // Include string.h for strcpy
#include "memory_allocator/memory_allocator.h"
#include <sys/mman.h>

#ifdef USE_CANARIES
#define MALLOC(size) mymalloc(size, __FILE__, __LINE__)
#define MY_ALLOC(T) (T*)memset(mymalloc(sizeof(T), __FILE__, __LINE__), 0, sizeof(T))
#else
#define MALLOC(size) mymalloc(size)
#define MY_ALLOC(T) (T*)memset(mymalloc(sizeof(T)), 0, sizeof(T))
#endif

#define FREE myfree


int main() {

    size_t memory_pool_size = 1024; //just large enough for test with seed 123
    initialize_memory_pool(memory_pool_size); 

     int *ptr1 = MY_ALLOC(int);
    *ptr1 = 10;
    printf("Value of ptr1: %d\n", *ptr1);

    FREE(ptr1);
    display_blocks();
    destroy_memory_pool();
    return 0;
}

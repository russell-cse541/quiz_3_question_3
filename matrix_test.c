#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // Include string.h for strcpy
#include "matrix_operations/matrix_operations.h"
#include <sys/mman.h>


#ifdef USE_MY_MALLOC
    #define MALLOC mymalloc
    #define FREE myfree
    #include "memory_allocator/memory_allocator.h"
#else
    #define MALLOC malloc
    #define FREE free
#endif

int main() {

#ifdef USE_MY_MALLOC
    initialize_memory_pool(40000*40000);
#endif
    int seed = 123;
    srand(seed);
    int testCount = 1;
    int modValue = 1000;
    int offsetValue = 500;
    int *randomSizes = (int*)MALLOC(testCount * sizeof(int));
    MatrixProduct** matrixProducts = (MatrixProduct**)MALLOC(testCount * sizeof(MatrixProduct*));
    if (randomSizes == NULL) {
        return 1;
    }
    for (int i = 0; i < testCount; i++) {
        randomSizes[i] = rand() % modValue + offsetValue;
    }
    for (int i=0; i<testCount; i++) {
        // int size = randomSizes[i];
        int size = 4000;
        matrixProducts[i] = buildMatrixTest(size);
        verifySuccess(matrixProducts[i]);
        if (matrixProducts[i] != NULL && matrixProducts[i]->success) {
            printf("Test successful for size %d\n", size);
        } else {
            printf("Test Failed for size %d\n", size);
        }
        if (i >= 2) {
            destroyMatrixProduct(matrixProducts[i-2]);
        }
    }

    if (testCount >= 2) {
        for (int i = testCount - 2; i < testCount; i++) {
            destroyMatrixProduct(matrixProducts[i]);
        }
    }

    FREE(randomSizes);
    FREE(matrixProducts);

    #ifdef USE_MY_MALLOC

    destroy_memory_pool();
    #endif

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    printf("Unit Tests For Some Edge Cases\n");
    // Test 1 Allocates 0 bytes
    printf("Test 1 Allocates 0 bytes\n");
    int *a = malloc(sizeof(char) * 0);
    if(a == NULL){
        printf("case passed\n\n");
    }else{
        printf("case not passed\n\n");
    }

    // Test 2 Allocate bytes that exceeds
    printf("Test 2 Allocate bytes that exceeds\n");
    int *b = malloc(sizeof(char) * 4097);
    if(b == NULL){
        printf("case passed\n\n");
    }else{
        printf("case not passed\n\n");
    }

    // Test 3 Trying to free when no allocations/pointer not belong to the memory
    printf("Test 3 Trying to free when no allocations/pointer not belong to the memory. With different pointers involved.\n");
    char c;
    int d;
    free(&c);
    free(&d);

    return EXIT_SUCCESS;
}
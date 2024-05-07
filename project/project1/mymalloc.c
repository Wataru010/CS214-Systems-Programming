#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.h"

#define MEMSIZE 4096
static char memory[MEMSIZE];

#define SECOND_HALF 255
#define HEADER_LENGTH 3
#define SIZE 1

void *mymalloc(size_t size, char *file, int line){

    if(size > MEMSIZE-HEADER_LENGTH || size <= 0){
        return NULL;
    }

    int used_or_not = 0;
    int location = 0;
    // int end_of_chunk = 0;
    if(memory[used_or_not] == 0){
        memory[used_or_not] = 't';
        unsigned short int temp_size = size;
        memcpy(&memory[used_or_not + SIZE], (char *)&temp_size, sizeof(unsigned short int));

        // for loop for memory visualization
        for(int index = used_or_not + HEADER_LENGTH; index < used_or_not + size + HEADER_LENGTH; index++){
            memory[index] = '*';
        }
        return (void *)&memory[used_or_not + HEADER_LENGTH];

    }else{
        while(location < MEMSIZE){
            if(memory[location] == 't'){
                unsigned short int size_from_memory = *(unsigned short int *)&memory[location + SIZE];
                location = location + HEADER_LENGTH + size_from_memory; // location + 1 + 3
            }else if(memory[location] == 'f'){
                unsigned short int size_from_memory = *(unsigned short int *)&memory[location + SIZE];
                if(size_from_memory >= size){
                    if(size_from_memory > size){
                        // change it to true for allocated
                        memory[location] = 't';

                        // store the new size information into the metadata
                        unsigned short int temp_size = size;
                        memcpy(&memory[location + SIZE], (char *)&temp_size, sizeof(unsigned short int));

                        // check the remain size of this chunk of memory
                        unsigned short int remaining_size = size_from_memory - size;

                        if(remaining_size <= 3){
                            if(remaining_size == 3){
                                memory[location + HEADER_LENGTH + size] = '3';
                            }else if(remaining_size == 2){
                                memory[location + HEADER_LENGTH + size] = '2';
                            }else if(remaining_size == 1){ // remaining_size == 1
                                memory[location + HEADER_LENGTH + size] = '1';                               
                            }
                        }else{
                            remaining_size = remaining_size - HEADER_LENGTH;
                            memory[location + HEADER_LENGTH + size] = 'f';
                            memcpy(&memory[location + HEADER_LENGTH + size + SIZE], (char *)&remaining_size, sizeof(unsigned short int));
                        }
                        return (void *)&memory[location + HEADER_LENGTH];
                    }else{
                        memory[location] = 't';
                        return (void *)&memory[location + HEADER_LENGTH];
                    }
                }else{
                    unsigned short int size_from_memory = *(unsigned short int *)&memory[location + SIZE];
                    location = location + HEADER_LENGTH + size_from_memory; // location + 1 + 3
                }
            }else{
                if(memory[location] == 0){
                    if(location + HEADER_LENGTH + size - 1 < MEMSIZE ){
                        memory[location] = 't';
                        unsigned short int temp_size = size;
                        memcpy(&memory[location + SIZE], (char *)&temp_size, sizeof(unsigned short int) );

                        // for loop for memory visualization
                        for(int index = location + HEADER_LENGTH; index < location + size + HEADER_LENGTH; index++){
                            memory[index] = '*';
                        }
                        return (void *)&memory[location + HEADER_LENGTH];
                    }else{
                        break;
                    }
                }else{
                    if(memory[location] == '3'){
                        location = location + 3;
                    }else if(memory[location] == '2'){
                        location = location + 2;
                    }else{ //memory[location] == '1
                        location = location + 1;
                    }
                }
            }
        }
        return NULL;
    }
}

void myfree(void *ptr, char *file, int line){
    int free_or_not = 0;

    // check the if the ptr is a good pointer
    int location = 0;
    int prev_ptr = 0;
    unsigned short int size_from_memory;
    while(location < MEMSIZE || memory[location] != 0){
        if(ptr == &(memory[location + HEADER_LENGTH])){
            // valid pointer
            if(memory[location] != 'f'){
                memory[location] = 'f';
                free_or_not = 1;
            }
            break;
        }
        if(memory[location] == '1' || memory[location] == '2' || memory[location] == '3'){
            prev_ptr = location;
            if(memory[location] == '3'){
                location = location + 3;
            }else if(memory[location] == '2'){
                location = location + 2;
            }else{ //memory[location] == '1
                location = location + 1;
            }
        }else{
            prev_ptr = location;
            size_from_memory = *(unsigned short int *)&memory[location + SIZE];
            location = location + HEADER_LENGTH + size_from_memory;
        }
    }   

    // coalasceing
    if(free_or_not == 1){      
        int is_the_head = 0;
        if(location == 0){
            is_the_head = 1;
        }
        int head_location = location;
        size_from_memory = *(unsigned short int *)&memory[location + SIZE];
        location = location + HEADER_LENGTH + size_from_memory;
        while(location < MEMSIZE || memory[location] != 0){
            if(memory[location] == 'f' || memory[location] == '1' || memory[location] == '2' || memory[location] == '3'){
                if(memory[location] == 'f')
                {
                    size_from_memory = *(unsigned short int *)&memory[location + SIZE];
                    size_from_memory = size_from_memory + HEADER_LENGTH;

                    unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
                    size_of_head_location = size_of_head_location + size_from_memory;
                    memcpy(&memory[head_location + SIZE], (char *)&size_of_head_location, sizeof(unsigned short int));
                    location = location + size_from_memory;
                }else if(memory[location] == '1'){
                    unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
                    size_of_head_location = size_of_head_location + 1;
                    memcpy(&memory[head_location + SIZE], (char *)&size_of_head_location, sizeof(unsigned short int));
                    location = location + 1;
                }else if(memory[location] == '2'){
                    unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
                    size_of_head_location = size_of_head_location + 2;
                    memcpy(&memory[head_location + SIZE], (char *)&size_of_head_location, sizeof(unsigned short int));
                    location = location + 2;
                }else if(memory[location] == '3'){
                    unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
                    size_of_head_location = size_of_head_location + 3;
                    memcpy(&memory[head_location + SIZE], (char *)&size_of_head_location, sizeof(unsigned short int));
                    location = location + 3;
                }
            }else if(memory[location] == 't'){
                break;
            }else{ // memmory[location] == '0'
                unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
                size_of_head_location = size_of_head_location + (MEMSIZE - location);
                memcpy(&memory[head_location + SIZE], (char *)&size_of_head_location, sizeof(unsigned short int));
                break;
            }
        }
        if(memory[prev_ptr] == 'f' && is_the_head != 1){
            unsigned short int size_of_head_location = *(unsigned short int *)&memory[head_location + SIZE];
            unsigned short int size_of_prev_ptr = *(unsigned short int *)&memory[prev_ptr + SIZE];
            size_of_prev_ptr = size_of_prev_ptr + size_of_head_location + HEADER_LENGTH;
            memcpy(&memory[prev_ptr + SIZE], (char *)&size_of_prev_ptr, sizeof(unsigned short int));
        }
    }

    // dealing with invalid pointer
    if(free_or_not == 0 ){
        printf("free: attempt to free non-block ptr (%s:%d)\n", file, line);
    }
}

// int main(int agrc, char **argv){ 
//     int *pointer1 = malloc(sizeof(char) * 2045);
//     int *pointer2 = malloc(sizeof(char) * 2045);

//     for(int index = 0; index < MEMSIZE; index++){
//         printf("%d %d\n", index, memory[index]);
//     }

//     free(pointer1);
//     free(pointer2);

//     for(int index = 0; index < MEMSIZE; index++){
//         printf("%d %d\n", index, memory[index]);
//     }

//     printf("%d\n", *(unsigned short int *)&memory[1]);

//     char *t;
//     char *ptr4[120];

//     for(int index = 0; index < 120; index++){
//         t = malloc(sizeof(char));
//         ptr4[index] = t;
//     }

//     for(int index = 0; index < MEMSIZE; index++){
//         printf("%d %d\n", index, memory[index]);
//     }

//     printf("%d\n", *(unsigned short int *)&memory[481]);

//     for(int index = 0; index < 120; index++){
//         free(ptr4[index]);
//     }

    


//     return EXIT_SUCCESS;
// }
	
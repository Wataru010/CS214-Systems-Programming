#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    struct timeval start, end;
    gettimeofday(&start, NULL);

    for(int index = 0; index < 50; index++){
        // memgrind #1
        int *p;
        for(int index = 0; index < 120; index++){
            p = malloc(sizeof(char));

            free(p);

        }
    }

    gettimeofday(&end, NULL);
    printf("memgrind test #1:\n");
    printf("Time taken to count to 10^5 is : %ld micro seconds\n",
    ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec)));


    gettimeofday(&start, NULL);

    for(int index = 0; index < 50; index++){
        // memgrind #2
        int *d;
        void *ptrs1[120];
        for(int index = 0; index < 120; index++){
            d = malloc(sizeof(char));
            ptrs1[index] = d;
        }

        for(int index = 0; index < 120; index++){
            free(ptrs1[index]);
        }
    }

    gettimeofday(&end, NULL);
    printf("memgrind test #2:\n");
    printf("Time taken to count to 10^5 is : %ld micro seconds\n",
    ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec)));


    gettimeofday(&start, NULL);

    for(int index = 0; index < 50; index++){
        // memgrind #3
        int *c;
        void *ptr2[120];
        int n = 0;
        
        int array_index = 0;
        while(array_index < 120){
            n = rand() % 2;
            if(n ==  1){
                c = malloc(sizeof(char));
                ptr2[array_index] = c;
                array_index++;
            }else if(n == 0){
                int a = rand()  % ((120 - 0 + 1) + 0);
                if(&ptr2[a] == 0){
                    free(ptr2[a]);
                    ptr2[a] = 0;
                }
            }
        }

        for(int index = 0; index < 120; index++){
            if(ptr2[index] != 0){
                    free(ptr2[index]);
            }
        }
    }

    gettimeofday(&end, NULL);
    printf("memgrind test #3:\n");
    printf("Time taken to count to 10^5 is : %ld micro seconds\n",
    ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec)));



    gettimeofday(&start, NULL);

    for(int index = 0; index < 50; index++){
        // memgrind #4
        // #1
        int *a = malloc(sizeof(char) * 4000);
        void *ptr3[120];

        free(a);

        for(int index = 0; index < 120; index++){
            a = malloc(sizeof(char));
            ptr3[index] = a;
        }

        for(int index = 0; index < 120; index++){
            free(ptr3[index]);
        }

        for(int index = 0; index < 120; index++){
            a = malloc(sizeof(char));
            ptr3[index] = a;
        }

        for(int index = 0; index < 120; index++){
            free(ptr3[index]);
        }
    }

    gettimeofday(&end, NULL);
    printf("memgrind test #4.1:\n");
    printf("Time taken to count to 10^5 is : %ld micro seconds\n",
    ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec)));

    
      gettimeofday(&start, NULL);

    for(int index = 0; index < 50; index++){
        // memgrind #4
        
        // #2
        int *ptr4[120];
        int *pointer1 = malloc(sizeof(char) * 2045);
        int *pointer2 = malloc(sizeof(char) * 2045);
        int *t;

        free(pointer1);
        free(pointer2);

        int *s = malloc(sizeof(char) * 4000);
        free(s);

        for(int index = 0; index < 120; index++){
            t = malloc(sizeof(char));
            ptr4[index] = t;
        }

        for(int index = 0; index < 120; index++){
            free(ptr4[index]);
        }
    }

    gettimeofday(&end, NULL);
    printf("memgrind test #4.2:\n");
    printf("Time taken to count to 10^5 is : %ld micro seconds\n",
    ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec)));

    
    return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>

int add(int, int);

int main(){
    printf("%d\n", add(1, 1));
    return EXIT_SUCCESS;
}

int add(int a, int b){
    return a + b;
}
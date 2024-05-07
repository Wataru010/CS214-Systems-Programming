#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    int curr_buffer_size = 512;
    char *b = (char*)malloc(curr_buffer_size);
    while(getcwd(b,curr_buffer_size) == NULL){
        curr_buffer_size = curr_buffer_size*2;
        b = (char *)realloc(b, curr_buffer_size);
    }
    int index = 0;
    while(b[index] !=  '\0'){
        index++;
    }
    if(index < curr_buffer_size-1){
        b[index] = '\n';
        b[index+1] = '\0';
        index=index+2;
    }else{
        curr_buffer_size = curr_buffer_size+2;
        b = (char *)realloc(b, curr_buffer_size);
        b[index] = '\n';
        b[index+1] = '\0';
        index=index+2;
    }
    printf("%s", b);
    fflush(stdout);
    free(b);
    return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define BUFSIZE 512

int main(int argc, char *argv[]){

    // if(argc == 1){
        // printf("%s\n", argv[0]);
    // }else{
        // for(int index = 0; index < argc; index++){
        //     printf("%s\n", argv[index]);
        // }  
    // }
    char buf[BUFSIZE];
    int byte = read(STDIN_FILENO, buf, BUFSIZE);
    write(STDOUT_FILENO, buf, byte);
    return EXIT_SUCCESS;
}
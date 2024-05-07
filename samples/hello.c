#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "example.h"

enum direction { north, south, east, west };
enum element { fire, earth, water, air };

typedef unsigned int age;

age foo;

int main(int argc, char **argv){
        int n = square(argc);
        
}

int old_main(int argc, char** argv)
{
        int i;
        printf("Hello, world!\n");

        printf("I go %d arguments!\n", argc);

        for (i = 0; i < argc; i++) {
                printf("Argument %d: |%s|\n", i, argv[i]);
        }
	
	enum direction departure = north;
	enum direction heading;
	
	if(heading == north) {
		printf("It was north!\n");
	} else {
		printf("Whoops!\n");
	}
        return EXIT_SUCCESS;
}


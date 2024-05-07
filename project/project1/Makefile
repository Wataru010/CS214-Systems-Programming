CC = gcc
CFLAGS  = -g -Wall -Wvla -Werror 

all: err memgrind test

test: test.o mymalloc.o
	$(CC) $(CFLAGS) -o test test.o mymalloc.o

err: err.o mymalloc.o
	$(CC) $(CFLAGS) -o err err.o mymalloc.o

memgrind: memgrind.o mymalloc.o 
	$(CC) $(CFLAGS) -o memgrind memgrind.o mymalloc.o

memgrind.o: memgrind.c mymalloc.c mymalloc.h 
	$(CC) $(CFLAGS) -c memgrind.c

mymalloc.o: mymalloc.c mymalloc.h 
	$(CC) $(CFLAGS) -c mymalloc.c

err.o: mymalloc.c err.c mymalloc.h
	$(CC) $(CFLAGS) -c err.c

test.o: mymalloc.c test.c mymalloc.h
	$(CC) $(CFLAGS) -c test.c

clean: 
	rm -f *.o err memgrind mymalloc test

CC = gcc
CFLAGS  = -g -std=c99 -Wall -Wvla -Werror -fsanitize=address,undefined -pthread

all: ttts ttt

ttts: ttts.o protocol.o
	$(CC) $(CFLAGS) -o ttts ttts.o protocol.o

ttts.o: ttts.c protocol.h 
	$(CC) $(CFLAGS) -c ttts.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

ttt: ttt.c
	$(CC) $(CFLAGS) -o ttt ttt.c

clean:
	rm -f *.o ttt ttts
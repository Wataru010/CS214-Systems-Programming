CC = gcc
CFLAGS  = -g -std=c99 -Wall -Wvla -Werror -fsanitize=address,undefined

all: mysh pwd_pipe redirection revline

mysh: mysh.c
	$(CC) $(CFLAGS) -o mysh mysh.c

pwd_pipe: pwd_pipe.c
	$(CC) $(CFLAGS) -o pwd_pipe pwd_pipe.c

redirection: redirection.c
	$(CC) $(CFLAGS) -o redirection redirection.c

revline: revline.c
	$(CC) $(CFLAGS) -o revline revline.c

clean:
	rm -f mysh revline test redirection bar baz some_file pwd_pipe *.txt

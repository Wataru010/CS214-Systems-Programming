#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#define BUFSIZE 512
pthread_mutex_t mutex_read;
pthread_mutex_t mutex_write;

int connection(char *host, char *port_number){
    struct addrinfo hint, *info_list, *info;
    int result, socket_0;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

    result = getaddrinfo(host, port_number, &hint, &info_list);
    if (result < 0) {
        fprintf(stderr, "error looking up %s:%s: %s\n", host, port_number, gai_strerror(result));
        return -1;
    }

    for(info = info_list; info != NULL; info = info->ai_next){
        socket_0 = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        if(socket_0 < 0) continue;

        result = connect(socket_0, info->ai_addr, info->ai_addrlen);

        if(result < 0){
            close(socket_0);
            continue;
        }

        break;
    }

    freeaddrinfo(info_list);

    if (info == NULL) {
        fprintf(stderr, "Unable to connect to %s:%s\n", host, port_number);
        return -1;
    }

    return socket_0;
}

void broken_pipe_handler(int sig){

    exit(EXIT_SUCCESS);
}



void *read_message_from_server(void *arg){
    int word_count;
    int socket_conn = *(int *)arg;
    int buf_read[BUFSIZE];

    // struct sigaction sa;
    // sa.sa_handler = &broken_pipe_handler;
    

    while ((word_count = read(socket_conn, buf_read, BUFSIZE)) > 0){
        // sigaction(SIGPIPE, &sa, NULL);
        signal(SIGPIPE, broken_pipe_handler);
        pthread_mutex_lock(&mutex_read);
        if(!(write(STDOUT_FILENO, buf_read, word_count) > 0)){
            break;
        }
        // write(STDOUT_FILENO, "\n", 2);
        pthread_mutex_unlock(&mutex_read);
    }
    // printf("read message from server done!");
    pthread_exit(NULL);

    return NULL;
}

void *write_to_server_from_client(void *arg){
    int word_count;
    int socket_conn = *(int *)arg;
    int buf_write[BUFSIZE];
    
    // struct sigaction sa;
    // sa.sa_handler = &broken_pipe_handler;
    // sigaction(SIGPIPE, &sa, NULL);

    while((word_count = read(STDIN_FILENO, buf_write, BUFSIZE)) > 0){
        // sigaction(SIGPIPE, &sa, NULL);
        signal(SIGPIPE, broken_pipe_handler);
        pthread_mutex_lock(&mutex_write);
        int check = write(socket_conn, buf_write, word_count);
        if(!(check > 0)){
            break;
        }
        pthread_mutex_unlock(&mutex_write);
    }
    // printf("read message from input done!");
    pthread_exit(NULL);

    return NULL;
}

int main(int argc, char *argv[]){
    int socket_in; 

    if(argc == 3){
        
        socket_in = connection(argv[1], argv[2]);
        if(socket_in < 0){
            printf("Connection failed! Check your input!\n");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_init(&mutex_read, NULL);
        pthread_mutex_init(&mutex_write, NULL);

        pthread_t read_from_server_thread, write_to_server_thread;
        pthread_create(&read_from_server_thread, NULL, read_message_from_server, &socket_in);
        pthread_create(&write_to_server_thread, NULL, write_to_server_from_client, &socket_in);

        // pthread_kill(read_from_server_thread, SIGPIPE);
        // pthread_kill(write_to_server_thread, SIGPIPE);
        if(pthread_join(read_from_server_thread, NULL) == 0 || pthread_join(write_to_server_thread, NULL) == 0){}
        
        // pthread_exit(NULL);
        pthread_mutex_destroy(&mutex_read);
        pthread_mutex_destroy(&mutex_write);

        close(socket_in);
    }else{
        printf("Enter the correct host and port number!");
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}
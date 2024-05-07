#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include "protocol.h"

#define BUFSIZE 512

char **user_name;
int count_socket = 0;
int sockets[2];
pthread_mutex_t mutex_add;
char *board;
int turn = 0;
int draw_state = 0;
int over_state = 0;
char role[2];

int listener(char *port_number, int queue_size){
    struct addrinfo hint, *info_list, *info;
    int result, socket_0;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port_number, &hint, &info_list);
    if(result != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for(info = info_list; info != NULL; info = info->ai_next){
        socket_0 = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        if(socket_0 < 0) continue;

        result = bind(socket_0, info->ai_addr, info->ai_addrlen);

        if(result == -1){
            close(socket_0);
            continue;
        }

        result = listen(socket_0, queue_size);

        if(result == -1){
            close(socket_0);
            continue;
        }

        break;
    }

    freeaddrinfo(info_list);

    if(info == NULL){
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    return socket_0;
}

int board_checking(char role){
    // board are fill with no one wins
    if(board[0] == role && board[1] == role && board[2] == role){
        return 1;
    }else if(board[3] == role && board[4] == role && board[5] == role){
        return 1;
    }else if(board[6] == role && board[7] == role && board[8] == role){
        return 1;
    }else if(board[0] == role && board[3] == role && board[6] == role){
        return 1;
    }else if(board[1] == role && board[4] == role && board[7] == role){
        return 1;
    }else if(board[2] == role && board[5] == role && board[8] == role){
        return 1;
    }else if(board[0] == role && board[4] == role && board[8] == role){
        return 1;
    }else if(board[2] == role && board[4] == role && board[6] == role){
        return 1;
    }else{
        for(int index = 0; index < strlen(board) - 1; index++){
            if(board[index] == '.'){
                return 0;
            }
        }
        return 2;
    }
    return 0;
}

void *p1_read_write(void *arg){
    int *sockets = (int *)arg;
    int player1_socket = sockets[0];
    int player2_socket = sockets[1];
    char buf1[BUFSIZE];
    int word_count1;
    msg *player1_message;
    char p1_msg_buf[BUFSIZE*2];

    while((word_count1 = read(player1_socket, buf1, BUFSIZE)) > 0){
        buf1[word_count1] = '\0';
        player1_message = tokenization(buf1);

        if(turn != 0){
            if(over_state == 1){
                free(player1_message);
                break;
            }
            if(strcmp(player1_message->protocol, "RSGN") != 0 && strcmp(player1_message->protocol, "DRAW") != 0){
                free(player1_message);
                continue;
            }
        }

        if(draw_state == 1){
            if(strcmp(player1_message->protocol, "DRAW") != 0){
                if(strcmp(player1_message->protocol, "RSGN") != 0){
                    sprintf(p1_msg_buf, "%s|%ld|%s\n", "INVL", strlen("You have to decide to draw or reject the draw!|"), "You have to decide to draw or reject the draw!|");

                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    // write(STDOUT_FILENO, "\n", 2);
                    write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                    memset(buf1, 0, BUFSIZE); 

                    free(player1_message);

                    memset(p1_msg_buf, 0, BUFSIZE*2);
                    continue;
                }
            }
        }

        if(strcmp(player1_message->protocol, "MOVE") == 0){
            move(player1_message, role[0], board);
        
            sprintf(p1_msg_buf, "%s|%d|%s\n", player1_message->protocol, player1_message->append_str_len, player1_message->str);
            write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
            write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
            // write(STDOUT_FILENO, "\n", 2);
            write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

            if(strcmp(player1_message->protocol, "MOVD") == 0){
                write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                turn = 1;
                int check = board_checking(role[0]);
                if(check == 1){
                    // make a buf to print the actually name
                    char buf_name[BUFSIZE];
                    strcpy(buf_name, user_name[0]);
                    buf_name[strlen(user_name[0])-1] = '\0';

                    sprintf(p1_msg_buf, "%s|%ld|%s|%s won!|\n", "OVER", strlen(buf_name) + strlen(" won!|") + 2, "W", buf_name);
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                    memset(p1_msg_buf, 0, BUFSIZE*2);

                    sprintf(p1_msg_buf, "%s|%ld|%s|%s won!|\n", "OVER", strlen(buf_name) + strlen(" won!|") + 2, "L", buf_name);
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    
                    write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                    
                    over_state = 1;
                }else if(check == 2){
                    sprintf(p1_msg_buf, "%s|%ld|%s|Game draw!|\n", "OVER", strlen("Game draw!|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                    memset(p1_msg_buf, 0, BUFSIZE*2);

                    sprintf(p1_msg_buf, "%s|%ld|%s|Game draw!|\n", "OVER", strlen("Game draw!|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    
                    write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                    
                    over_state = 1;
                }
            }
            
            memset(buf1, 0, BUFSIZE);
            
            free(player1_message);

            memset(p1_msg_buf, 0, BUFSIZE*2);
            
        }else if(strcmp(player1_message->protocol, "RSGN") == 0){
            // make a buf to print the actually name
            char buf_name[BUFSIZE];
            strcpy(buf_name, user_name[0]);
            buf_name[strlen(user_name[0])-1] = '\0';

            sprintf(p1_msg_buf, "%s|%ld|%s|%s has resigned.|\n", "OVER", strlen(buf_name) + strlen(" has resigned.|") + 2, "L", buf_name);
            write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
            write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
            write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

            memset(p1_msg_buf, 0, BUFSIZE*2);

            sprintf(p1_msg_buf, "%s|%ld|%s|%s has resigned.|\n", "OVER", strlen(buf_name) + strlen(" has resigned.|") + 2, "W", buf_name);
            write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
            write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
            
            write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
            free(player1_message);
            over_state = 1;
            
            // exit(EXIT_SUCCESS);
        }else if(strcmp(player1_message->protocol, "DRAW") == 0){
            draw(player1_message, draw_state);

            if(strcmp(player1_message->protocol, "INVL") == 0){
                sprintf(p1_msg_buf, "%s|%d|%s\n", player1_message->protocol, player1_message->append_str_len, player1_message->str);
                
                write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                // write(STDOUT_FILENO, "\n", 2);
                write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                
                memset(buf1, 0, BUFSIZE); 

                free(player1_message);

                memset(p1_msg_buf, 0, BUFSIZE*2);
            }else{
                if(strcmp(player1_message->str, "S|") == 0){
                    sprintf(p1_msg_buf, "%s|%d|%s\n", player1_message->protocol, player1_message->append_str_len, player1_message->str);
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    
                    write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                    memset(buf1, 0, BUFSIZE);
                    free(player1_message);
                    memset(p1_msg_buf, 0, BUFSIZE*2);
                    // turn = 1;
                    draw_state = 1;
                }else if(strcmp(player1_message->str, "A|") == 0){
                    sprintf(p1_msg_buf, "%s|%ld|%s|Both player agreed on draw.|\n", "OVER", strlen("Both player agreed on draw.|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                    memset(p1_msg_buf, 0, BUFSIZE*2);

                    sprintf(p1_msg_buf, "%s|%ld|%s|Both player agreed on draw.|\n", "OVER", strlen("Both player agreed on draw.|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    
                    write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                    free(player1_message);
                    over_state = 1;
                    
                }else{ // strcmp(player1_message->str, "R|") == 0
                    sprintf(p1_msg_buf, "%s|%d|%s\n", player1_message->protocol, player1_message->append_str_len, player1_message->str);
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                    
                    write(player2_socket, p1_msg_buf, strlen(p1_msg_buf));
                    memset(buf1, 0, BUFSIZE);
                    free(player1_message);
                    memset(p1_msg_buf, 0, BUFSIZE*2);
                    // turn = 1;
                    draw_state = 0;
                }
            }
            
        }else{
            if(strcmp(player1_message->protocol, "PLAY") == 0){
                sprintf(p1_msg_buf, "%s|%ld|%s\n", "INVL", strlen("Game has started!|"), "Game has started!|");

                write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                // write(STDOUT_FILENO, "\n", 2);
                write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                
                memset(buf1, 0, BUFSIZE); 

                free(player1_message);

                memset(p1_msg_buf, 0, BUFSIZE*2);
            }else if(strcmp(player1_message->protocol, "INVL") == 0){
                
                sprintf(p1_msg_buf, "%s|%d|%s\n", player1_message->protocol, player1_message->append_str_len, player1_message->str);
                
                write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                write(STDOUT_FILENO, p1_msg_buf, strlen(p1_msg_buf));
                // write(STDOUT_FILENO, "\n", 2);
                write(player1_socket, p1_msg_buf, strlen(p1_msg_buf));

                
                memset(buf1, 0, BUFSIZE); 

                free(player1_message);

                memset(p1_msg_buf, 0, BUFSIZE*2);
            }
        }
        // write(STDOUT_FILENO, buf1, word_count1);
        // write(player2_socket, buf1, word_count1);
        // memset(buf1, 0, BUFSIZE);
        if(over_state == 1){
            // pthread_exit(0);
            printf("Game set exiting 1 ... \n");
            pthread_exit(0);
            // exit(EXIT_SUCCESS);
            break;
        }
    }
    // pthread_exit(0);
    return NULL;
}

void *p2_read_write(void *arg){
    int *sockets = (int *)arg;
    int player1_socket = sockets[0];
    int player2_socket = sockets[1];
    char buf2[BUFSIZE];
    int word_count2;
    msg *player2_message;
    char p2_msg_buf[BUFSIZE*2];

    while((word_count2 = read(player2_socket, buf2, BUFSIZE)) > 0){
        buf2[word_count2] = '\0';
        player2_message = tokenization(buf2);

        if(turn != 1){
            if(over_state == 1){
                free(player2_message);
                break;
            }
            if(strcmp(player2_message->protocol, "RSGN") != 0 && strcmp(player2_message ->protocol, "DRAW") != 0){
                free(player2_message);
                continue;
            }
        }

        if(draw_state == 1){
            if(strcmp(player2_message->protocol, "DRAW") != 0){
                if(strcmp(player2_message->protocol, "RSGN") != 0){
                    sprintf(p2_msg_buf, "%s|%ld|%s\n", "INVL", strlen("You have to decide to draw or reject the draw!|"), "You have to decide to draw or reject the draw!|");

                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    // write(STDOUT_FILENO, "\n", 2);
                    write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                    memset(buf2, 0, BUFSIZE); 

                    free(player2_message);

                    memset(p2_msg_buf, 0, BUFSIZE*2);
                    continue;
                }
            }
        }

        if(strcmp(player2_message->protocol, "MOVE") == 0){
            move(player2_message, role[1], board);
        
            sprintf(p2_msg_buf, "%s|%d|%s\n", player2_message->protocol, player2_message->append_str_len, player2_message->str);
            write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
            write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
            // write(STDOUT_FILENO, "\n", 2);
            write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

            if(strcmp(player2_message->protocol, "MOVD") == 0){
                write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
                turn = 0;
                int check = board_checking(role[0]);
                if(check == 1){
                    // make a buf to print the actually name
                    char buf_name[BUFSIZE];
                    strcpy(buf_name, user_name[1]);
                    buf_name[strlen(user_name[1])-1] = '\0';

                    sprintf(p2_msg_buf, "%s|%ld|%s|%s won!|\n", "OVER", strlen(buf_name) + strlen(" won!|") + 2, "W", buf_name);
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                    memset(p2_msg_buf, 0, BUFSIZE*2);

                    sprintf(p2_msg_buf, "%s|%ld|%s|%s won!|\n", "OVER", strlen(buf_name) + strlen(" won!|") + 2, "L", buf_name);
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    
                    write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
                    
                    over_state = 1;
                }else if(check == 2){
                    sprintf(p2_msg_buf, "%s|%ld|%s|Game draw!|\n", "OVER", strlen("Game draw!|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                    memset(p2_msg_buf, 0, BUFSIZE*2);

                    sprintf(p2_msg_buf, "%s|%ld|%s|Game draw!|\n", "OVER", strlen("Game draw!|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    
                    write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
                    
                    over_state = 1;
                }
            }
            
            memset(buf2, 0, BUFSIZE);
            
            free(player2_message);

            memset(p2_msg_buf, 0, BUFSIZE*2);
            
        }else if(strcmp(player2_message->protocol, "RSGN") == 0){
            // make a buf to print the actually name
            char buf_name[BUFSIZE];
            strcpy(buf_name, user_name[1]);
            buf_name[strlen(user_name[1])-1] = '\0';

            sprintf(p2_msg_buf, "%s|%ld|%s|%s has resigned.|\n", "OVER", strlen(buf_name) + strlen(" has resigned.|") + 2, "L", buf_name);
            write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
            write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
            write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

            memset(p2_msg_buf, 0, BUFSIZE*2);

            sprintf(p2_msg_buf, "%s|%ld|%s|%s has resigned.|\n", "OVER", strlen(buf_name) + strlen(" has resigned.|") + 2, "W", buf_name);
            write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
            write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
            
            write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
            free(player2_message);
            over_state = 1;
            
            // exit(EXIT_SUCCESS);
        }else if(strcmp(player2_message->protocol, "DRAW") == 0){
            draw(player2_message, draw_state);

            if(strcmp(player2_message->protocol, "INVL") == 0){
                sprintf(p2_msg_buf, "%s|%d|%s\n", player2_message->protocol, player2_message->append_str_len, player2_message->str);
                
                write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
    
                write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                
                memset(buf2, 0, BUFSIZE); 

                free(player2_message);

                memset(p2_msg_buf, 0, BUFSIZE*2);
            }else{
                if(strcmp(player2_message->str, "S|") == 0){
                    sprintf(p2_msg_buf, "%s|%d|%s\n", player2_message->protocol, player2_message->append_str_len, player2_message->str);
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    
                    write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
                    memset(buf2, 0, BUFSIZE);
                    free(player2_message);
                    memset(p2_msg_buf, 0, BUFSIZE*2);
                    // turn = 0;
                    draw_state = 1;
                }else if(strcmp(player2_message->str, "A|") == 0){
                    sprintf(p2_msg_buf, "%s|%ld|%s|Both player agreed on draw.|\n", "OVER", strlen("Both player agreed on draw.|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));

                    memset(p2_msg_buf, 0, BUFSIZE*2);

                    sprintf(p2_msg_buf, "%s|%ld|%s|Both player agreed on draw.|\n", "OVER", strlen("Both player agreed on draw.|") + 2, "D");
                    write(STDOUT_FILENO, "TO PLAYER 2 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    
                    write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));
                    free(player2_message);
                    over_state = 1;
            
                }else{ // strcmp(player1_message->str, "R|") == 0
                    sprintf(p2_msg_buf, "%s|%d|%s\n", player2_message->protocol, player2_message->append_str_len, player2_message->str);
                    write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                    write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                    
                    write(player1_socket, p2_msg_buf, strlen(p2_msg_buf));
                    memset(buf2, 0, BUFSIZE);
                    free(player2_message);
                    memset(p2_msg_buf, 0, BUFSIZE*2);
                    // turn = 0;
                    draw_state = 0;
                }
            }
        }else{
            if(strcmp(player2_message->protocol, "PLAY") == 0){
                sprintf(p2_msg_buf, "%s|%ld|%s\n", "INVL", strlen("Game has started!|"), "Game has started!|");

                write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                // write(STDOUT_FILENO, "\n", 2);
                write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                
                memset(buf2, 0, BUFSIZE); 

                free(player2_message);

                memset(p2_msg_buf, 0, BUFSIZE*2);
            }else if(strcmp(player2_message->protocol, "INVL") == 0){
                
                sprintf(p2_msg_buf, "%s|%d|%s\n", player2_message->protocol, player2_message->append_str_len, player2_message->str);
                
                write(STDOUT_FILENO, "TO PLAYER 1 -> ", 16);
                write(STDOUT_FILENO, p2_msg_buf, strlen(p2_msg_buf));
                // write(STDOUT_FILENO, "\n", 2);
                write(player2_socket, p2_msg_buf, strlen(p2_msg_buf));

                
                memset(buf2, 0, BUFSIZE); 

                free(player2_message);

                memset(p2_msg_buf, 0, BUFSIZE*2);
            }
        }
        
        // write(STDOUT_FILENO, buf2, word_count2);
        // write(player1_socket, buf2, word_count2);
        // memset(buf2, 0, BUFSIZE);

        if(over_state == 1){
            // pthread_exit(0);
            printf("Game set exiting 2 ... \n");
            pthread_exit(0);
            // exit(EXIT_SUCCESS);
            break;
        }
    }
    // pthread_exit(0);
    return NULL;
}

void *setup(void *arg){
    int socket_temp = *(int *)arg;
    char buf[BUFSIZE];
    int input_count;
    msg *player;
    int same_username_check = 0;
    while((input_count = read(socket_temp, buf, BUFSIZE)) > 0){
        write(STDOUT_FILENO, buf, input_count);
        buf[input_count] = '\0';
        player = tokenization(buf);
        memset(buf, 0, BUFSIZE);

        if(strcmp(player->protocol, "PLAY") != 0){
            char str_buf1[BUFSIZE*2];
            sprintf(str_buf1, "%s|%ld|%s\n", "INVL", strlen("Please enter name informat of PLAY|5|NAME| to start game|"), "Please enter name informat of PLAY|5|NAME| to start game|");
            write(STDOUT_FILENO, str_buf1, strlen(str_buf1)); // write(STDOUT_FILENO, "\n", 2);
            write(socket_temp, str_buf1, strlen(str_buf1));
        }
        
        memset(buf, 0, BUFSIZE);
        if(strcmp(player->protocol, "PLAY") == 0){
            user_name[count_socket] = malloc(sizeof(char) * player->append_str_len+1);
            strcpy(user_name[count_socket], player->str);
            same_username_check = 0;
            pthread_mutex_lock(&mutex_add);
            for(int index = 0; index < count_socket; index++){
                if(strcmp(user_name[index], user_name[count_socket]) == 0){
                    same_username_check = 1;
                    strcpy(player->protocol, "INVL");
                    memset(player->str, 0, BUFSIZE);
                    strcpy(player->str, "name already in use!|");
                    player->append_str_len = strlen(player->str);

                    char str_buf2[BUFSIZE*2];
                    sprintf(str_buf2, "%s|%d|%s\n", player->protocol, player->append_str_len, player->str);
                    write(STDOUT_FILENO, str_buf2, strlen(str_buf2)); write(STDOUT_FILENO, "\n", 2);
                    write(socket_temp, str_buf2, strlen(str_buf2));
                    free(user_name[count_socket]);
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_add);
        }

        if(strcmp(player->protocol, "PLAY") == 0 && same_username_check == 0){
            free(player);
            break;
        }
        free(player);
    }
    
    same_username_check = 0;
    write(socket_temp, "WAIT|0|\n", 9);
    sockets[count_socket] = socket_temp; 
    pthread_mutex_lock(&mutex_add);
    count_socket++;
    // printf("%d\n", count_socket);
    pthread_mutex_unlock(&mutex_add);
    pthread_exit(NULL);
    return NULL;
}

void *to_start_game(void *arg){
    return NULL;
}

int main(int argc, char *argv[]){
    
    if(argc > 1){
        struct sockaddr_storage host_addr;
        socklen_t host_addr_len;

        char *port_number = argv[1];

        int socket_conn = listener(port_number, 1);

        if(socket_conn < 0){
            printf("Fail to connect!");
            exit(EXIT_FAILURE);
        }

        // int count_socket = 0;

        // int sockets[2];
        // int num_connection = 0;
        user_name = malloc(sizeof(char *)*2);

        int socket_temp;

        write(STDOUT_FILENO, "Waiting for players ... \n", 26);

        pthread_mutex_init(&mutex_add, NULL);
        pthread_t user_threads[2];
        int count_thread = 0;

        while (count_thread != 2)
        {
            host_addr_len = sizeof(host_addr);
            socket_temp = accept(socket_conn, (struct sockaddr *)&host_addr, &host_addr_len);

            if(socket_temp < 0){
                perror("accept");
                continue;
            }

            sockets[count_thread] = socket_temp;
            pthread_t setup_user_thread;
            pthread_create(&setup_user_thread, NULL, setup, (void *)&socket_temp);
            // pthread_detach(setup_user_thread);
            user_threads[count_thread] = setup_user_thread;
            count_thread++;
            
        }
        
        pthread_mutex_destroy(&mutex_add);

        for(int index = 0; index < 2; index++){
            pthread_join(user_threads[index], NULL);
        }

        int player1_socket, player2_socket;
        // char role[2];
        // long int random = (rand()&1);
        // printf("%ld\n", random);
        player1_socket = sockets[0]; player2_socket = sockets[1];
        char buf1[BUFSIZE] = "BEGN|X|";
        strcpy(buf1+7, user_name[1]); strcat(buf1, "\n");
        write(player1_socket, buf1, 7+strlen(user_name[1])+1);
        char buf2[BUFSIZE] = "BEGN|O|";
        strcpy(buf2+7, user_name[0]); strcat(buf2, "\n");
        write(player2_socket, buf2, 7+strlen(user_name[0])+1);
        role[0] = 'X'; role[1] = 'O';
        // role[0] = 'X'; role[1] = 'O';
        // if(random == 0){
        //     player1_socket = sockets[0]; player2_socket = sockets[1];
        //     char buf1[BUFSIZE] = "BEGN|X|";
        //     strcpy(buf1+7, user_name[1]); strcat(buf1, "\n");
        //     write(player1_socket, buf1, 7+strlen(user_name[1])+1);
        //     char buf2[BUFSIZE] = "BEGN|O|";
        //     strcpy(buf2+7, user_name[0]); strcat(buf2, "\n");
        //     write(player2_socket, buf2, 7+strlen(user_name[0])+1);
        //     // role[0] = 'X'; role[1] = 'O';
        // }else{
        //     player1_socket = sockets[1]; player2_socket = sockets[0];
        //     char buf1[BUFSIZE] = "BEGN|X|";
        //     strcpy(buf1+7, user_name[0]); strcat(buf1, "\n");
        //     write(player1_socket, buf1, 7+strlen(user_name[0])+1);
        //     char buf2[BUFSIZE] = "BEGN|O|";
        //     strcpy(buf2+7, user_name[1]); strcat(buf2, "\n");
        //     write(player2_socket, buf2, 7+strlen(user_name[1])+1);
        //     // role[0] = 'O'; role[1] = 'X';
        // }
        
        // setup board
        board = malloc(sizeof(char)*11); memset(board, '.', 9); board[9] = '|'; board[10] = '\0';
        
        pthread_t p1_thread, p2_thread;
        pthread_create(&p1_thread, NULL, p1_read_write, (void *)sockets); // pthread_detach(p1_thread);
        pthread_create(&p2_thread, NULL, p2_read_write, (void *)sockets); // pthread_detach(p2_thread);

        // pthread_cancel(p1_thread);
        if(pthread_join(p1_thread, NULL) == 0 || pthread_join(p2_thread, NULL) == 0){}
        // pthread_join(p1_thread, NULL);
        // close(player1_socket);
        // pthread_cancel(p2_thread);
        // pthread_join(p2_thread, NULL);
        // close(player2_socket);

        close(player1_socket);
        close(player2_socket);
        close(socket_conn);
        free(user_name[0]);
        free(user_name[1]);
        free(user_name);
        free(board);
    }else{
        printf("Please restart the server and provide a port number!\n");
    }
    // pthread_exit(0);
    printf("Game over - exiting ... \n");
    
    // pthread_exit(0);
    return EXIT_SUCCESS;
}

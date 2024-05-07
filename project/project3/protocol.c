#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>  
#include "protocol.h"

// PLAY|6|Peter|
    // 1. check if it is invalid
    // 2. if the name is too long
// MOVE|6|X|2,2|
    // 1. check if it is invalid
    // 2. check if the spot has been chosen
    // MOVE|6|X|2,2|
        // 1. check role
        // 2. check format '|' after role
        // 3. check postion valid or not, single char number separate by comma
        // 4. check availability, if the postion is occupied or not
        // 5. if all works then make move i.e. MOVD
    // MOVD|16|X|2,2|....X....|
    // INVL|24|That space is occupied.|
// RSGN|0|
    // 1. send to both player with over and who resigned.
    // 2. if it is invalid
    // OVER|26|W|Joe Smith has resigned.|
// DRAW|2|S|
// DRAW|2|A|
    // OVER|26|W|Joe Smith has resigned.|
// DRAW|2|R|
    // 1. if draw succeed game stop and send over to both player with draw state
    // 2. if draw failed game continues
    // 3. if it is invalid


// how to achieve this protocol setting
// 1. tokenization
// 2. check valid or not
// 3. act accordingly and send back result

// maybe a struct
    // char protocol[5]
    // int append_str_len
    // char str[BUFSIZE]

// #define BUFSIZE 512

// typedef struct message
// {   
//     char protocol[5];
//     int append_str_len;
//     char str[BUFSIZE];
// }msg;

msg *tokenization(char *input_str){
    msg *message = NULL;
    if(strlen(input_str) >= 7){
        message = (msg *)malloc(sizeof(msg));
        char prot_buf[6];
        memcpy(prot_buf, input_str, 5);
        prot_buf[5] = '\0';

        if(prot_buf[4] == '|'){
            if(strcmp(prot_buf, "PLAY|") == 0 || strcmp(prot_buf, "MOVE|") == 0 || strcmp(prot_buf, "RSGN|") == 0 || strcmp(prot_buf, "DRAW|") == 0){
                prot_buf[4] = '\0';
                strcpy(message->protocol, prot_buf);

                char length_buf[5];
                char *ptr = input_str+5;
                int index = 0;
                while(ptr[index] != '|'){
                    length_buf[index] = ptr[index];
                    index++;
                    if(index >= 4){
                        // invalid message
                        char *prot = "INVL";
                        strcpy(message->protocol, prot);
                        char *msg_str = "Invalid input message too long or length doesn't match!|";
                        strcpy(message->str, msg_str);
                        message->append_str_len = strlen(msg_str);
                        return message;
                    }
                }
                length_buf[index] = '\0';
                message->append_str_len = atoi(length_buf);

                ptr = ptr+index+1;
                if(strlen(ptr) != message->append_str_len){
                    // invalid message
                    char *prot = "INVL";
                    strcpy(message->protocol, prot);
                    char *msg_str = "Invalid input message length doesn't match!|";
                    strcpy(message->str, msg_str);
                    message->append_str_len = strlen(msg_str);
                    return message;
                }

                if(message->append_str_len == 0 && strlen(length_buf) != 1){
                    // invalid message
                    char *prot = "INVL";
                    strcpy(message->protocol, prot);
                    char *msg_str = "Invalid input message length doesn't match!|";
                    strcpy(message->str, msg_str);
                    message->append_str_len = strlen(msg_str);
                    return message;
                }

                strcpy(message->str, ptr);
                if(message->str[strlen(message->str)-1] != '|' && strcmp(message->protocol, "RSGN") != 0){
                    // invalid message
                    char *prot = "INVL";
                    strcpy(message->protocol, prot);
                    char *msg_str = "Invalid input message format wrong!|";
                    strcpy(message->str, msg_str);
                    message->append_str_len = strlen(msg_str);
                    return message;
                }

            }else{
                // invalid message
                char *prot = "INVL";
                strcpy(message->protocol, prot);
                char *msg_str = "Invalid input wrong command!|";
                strcpy(message->str, msg_str);
                message->append_str_len = strlen(msg_str);
                return message;
            }
        }else{
            // invalid message
            char *prot = "INVL";
            strcpy(message->protocol, prot);
            char *msg_str = "Invalid input wrong command format!|";
            strcpy(message->str, msg_str);
            message->append_str_len = strlen(msg_str);
            return message;
        }
    }else{
        message = (msg *)malloc(sizeof(msg));
        char *prot = "INVL";
        strcpy(message->protocol, prot);
        char *msg_str = "Invalid input message might be too short!|";
        strcpy(message->str, msg_str);
        message->append_str_len = strlen(msg_str);
    }
    return message;
}

int move(msg *message, char role, char *board){
    char *msg_command = message->protocol;
    if(strcmp(msg_command, "MOVE") != 0){
        // invalid input
        
    }

    char *msg_str = message->str;
    if(message->append_str_len != 6){
        // invalid input not align with input format
        strcpy(message->protocol, "INVL");
        memset(message->str, 0, BUFSIZE);
        strcpy(message->str, "Invalid MOVE reenter please!|");
        message->append_str_len = strlen(message->str);
        return 1;
    }

    if(msg_str[0] == 'X' || msg_str[0] == 'O'){
        if(msg_str[0] == role){
            if(msg_str[1] == '|'){
                char row = msg_str[2];
                char col = msg_str[4];
                char delimiter = msg_str[3];

                if((row < 52 && row > 48) && (col < 52 && col > 48) && delimiter == ','){
                    if(board[((row-48)-1)*3 +(col-48)-1] == '.'){
                        board[((row-48)-1)*3 +(col-48)-1] = role;
                        strcpy(message->protocol, "MOVD");
                        char temp_buf[BUFSIZE];
                        strcpy(temp_buf, message->str);
                        strcat(temp_buf, board);
                        strcpy(message->str, temp_buf);
                        message->append_str_len = strlen(message->str);
                        return 1;
                    }else{
                        // invalid input position occupied
                        strcpy(message->protocol, "INVL");
                        memset(message->str, 0, BUFSIZE);
                        strcpy(message->str, "That space is occupied.|");
                        message->append_str_len = strlen(message->str);
                        return 1;
                    }
                }else{
                    // invalid input position on board
                    strcpy(message->protocol, "INVL");
                    memset(message->str, 0, BUFSIZE);
                    strcpy(message->str, "Invalid input position on board!|");
                    message->append_str_len = strlen(message->str);
                    return 1;
                }
            }else{
                // invalid input not align with input format
                strcpy(message->protocol, "INVL");
                memset(message->str, 0, BUFSIZE);
                strcpy(message->str, "Invalid input not align with input format!|");
                message->append_str_len = strlen(message->str);
                return 1;
            }   
        }else{
            // invalid input wrong role between X and O
            strcpy(message->protocol, "INVL");
            memset(message->str, 0, BUFSIZE);
            strcpy(message->str, "Invalid input this is not your role!|");
            message->append_str_len = strlen(message->str);
            return 1;
        }
    }else{
        // invalid input wrong role
        strcpy(message->protocol, "INVL");
        memset(message->str, 0, BUFSIZE);
        strcpy(message->str, "Invalid input not such role!|");
        message->append_str_len = strlen(message->str);
        return 1;
    }
}

int draw(msg *message, int draw_state){
    if(draw_state == 0){
        if(message->append_str_len > 2){
            // invalid draw message
            strcpy(message->protocol, "INVL");
            memset(message->str, 0, BUFSIZE);
            strcpy(message->str, "Invalid input wrong format of draw message!|");
            message->append_str_len = strlen(message->str);
            return 1;
        }else{
            if(strcmp(message->str, "S|") != 0){
                // invalid input your opponent didn't suggest a draw
                strcpy(message->protocol, "INVL");
                memset(message->str, 0, BUFSIZE);
                strcpy(message->str, "Invalid input your opponent didn't suggest a draw!|");
                message->append_str_len = strlen(message->str);
                return 1;
            }
        }
    }else{ // draw_state == 1
        if(message->append_str_len > 2){
            // invalid draw message
            strcpy(message->protocol, "INVL");
            memset(message->str, 0, BUFSIZE);
            strcpy(message->str, "Invalid input wrong format of draw message!|");
            message->append_str_len = strlen(message->str);
            return 1;
        }else{
            if(strcmp(message->str, "A|") != 0 && strcmp(message->str, "R|") != 0){
                // invalid input make decision on agree or reject
                strcpy(message->protocol, "INVL");
                memset(message->str, 0, BUFSIZE);
                strcpy(message->str, "Invalid input make decision on agree or reject the draw suggest!|");
                message->append_str_len = strlen(message->str);
                return 1;
                
            }
        }
    }
    return 0;
}

void resigned(msg *message, char *user_resigned){
    if(message->append_str_len != 0){
        strcpy(message->protocol, "INVL");
        memset(message->str, 0, BUFSIZE);
        strcpy(message->str, "Resigned form wrong! Correct form \"RSGN|0|\" |");
        message->append_str_len = strlen(message->str);
    }
}

/*
int main(){
    // gcc -g -std=c99 -Wall -Wvla -Werror -fsanitize=address,undefined -o prot protocol.c
    char buf[BUFSIZE];
    int count;

    if((count = read(STDIN_FILENO, buf, BUFSIZE)) > 0){
        buf[count-1] = '\0';
        msg *ptr = tokenization(buf);
        printf("%s:%ld:%d\n", buf, strlen(buf),count);
        printf("protocol: %s\n", ptr->protocol);
        printf("message length : %d\n", ptr->append_str_len);
        printf("message: %s\n", ptr->str);
        char str_buf[BUFSIZE*2];
        sprintf(str_buf, "%s|%d|%s", ptr->protocol, ptr->append_str_len, ptr->str);
        printf("complete message: %s : %ld\n", str_buf, strlen(str_buf));
        printf("%d\n", strcmp("INVL", ptr->protocol));

        printf("\n");

        char *board = malloc(sizeof(char)*11);
        memset(board, '.', 9);
        board[9] = '|';
        board[10] = '\0';
        memset(str_buf, 0, BUFSIZE*2);
        move(ptr, 'X', board);
        printf("%s:%ld:%d\n", buf, strlen(buf),count);
        printf("protocol: %s\n", ptr->protocol);
        printf("message length : %d\n", ptr->append_str_len);
        printf("message: %s\n", ptr->str);
        sprintf(str_buf, "%s|%d|%s", ptr->protocol, ptr->append_str_len, ptr->str);
        printf("complete message: %s : %ld\n", str_buf, strlen(str_buf));
        free(board);
        free(ptr);
    }
    return EXIT_SUCCESS;
}
*/
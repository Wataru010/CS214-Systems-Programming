#ifndef _PROTOCOL_H
#define _PROTOCOL_H
    #define BUFSIZE 512
    typedef struct message
    {   
        char protocol[5];
        int append_str_len;
        char str[BUFSIZE];
    }msg;

    msg *tokenization(char *input_str);

    int move(msg *message, char role, char *board);

    int draw(msg *message, int draw_state);
#endif
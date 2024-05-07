#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <glob.h>

#define BUFSIZE 512
#define FILE_BUFSIZE 1
#define PROMPTSIZE_EXIT_SUCCESS 6
#define PROMPTSIZE_EXIT_FAILURE 7
#define NO_REDIRECTION_PWD 0
#define REDIRECTION_PWD 1

typedef struct command_list
{
    struct command_list *prev;
    char *token;
    struct command_list *next;
}commands;

typedef struct argument_list
{
    char *arg;
    struct argument_list *next;
}arguments;

typedef struct instruction_list{
    char *exe_command;
    arguments *arguments;
    char *std_in;
    char *std_out;
    struct instruction_list *next;
}instructions;

typedef struct file_lines{
    char *line;
    struct file_lines *nextline;
}lines;

void print_arguments(arguments *);
void print_instruction(instructions *);
void free_arguments(arguments *);
void free_instruction(instructions *);
int find_file_and_execute(instructions *);

arguments *argument_wildcard(){
    return NULL;
}

char *exe_command_wildcard(){
    return NULL;
}

int cd(char * path){
    if(path == NULL){
        if(chdir(getenv("HOME")) == -1){
            perror("cd");
            return 1;
        }
    }else{
        if(path[0] == '~'){
            if(chdir(getenv("HOME")) != -1){
                char *new_path = malloc(sizeof(char) * (strlen(path) - 2) + 1);
                new_path[sizeof(char) * (strlen(path) - 2)] = '\0';
                int count = 2;
                for(int index = 0; index < (strlen(path) - 2) + 1; index++){
                    new_path[index] = path[count];
                    count++;
                }
                printf("%s\n", new_path);
                fflush(stdout);
                if(chdir(new_path) == -1){
                    perror("cd");
                    return 1;
                }
            }
        }else{
            if(chdir(path) == -1){
                perror("cd");
                return 1;
            }
        }
    }
    return 0;
}

char * pwd(int mode){
    int curr_buffer_size = BUFSIZE;
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
    if(mode == NO_REDIRECTION_PWD){
        printf("%s", b);
    }
    return b;
}

void exit_mysh(){
    write(STDOUT_FILENO,"mysh: exiting", 14);
    write(STDOUT_FILENO, "\n", 1);
    exit(EXIT_SUCCESS);
}

void generate_argument_node(instructions *instruct_ptr, commands *command_ptr){
    arguments *argument_node = (arguments *)malloc(sizeof(arguments));
    argument_node->arg = command_ptr->token; 
    argument_node->next = NULL;
    if(instruct_ptr->arguments == NULL){
        instruct_ptr->arguments = argument_node;
    }else{
        arguments *end = instruct_ptr->arguments;
        while(end->next != NULL){
            end = end->next;
        }
        end->next = argument_node;
    }
}

commands *generate_argument_node_for_wildcard(instructions *instruct_ptr, char *matched_file){
    char *file = (char *)malloc(sizeof(char) * strlen(matched_file) + 1);
    strcpy(file, matched_file);
    arguments *argument_node = (arguments *)malloc(sizeof(arguments));
    argument_node->arg = file; 
    argument_node->next = NULL;
    if(instruct_ptr->arguments == NULL){
        instruct_ptr->arguments = argument_node;
    }else{
        arguments *end = instruct_ptr->arguments;
        while(end->next != NULL){
            end = end->next;
        }
        end->next = argument_node;
    }
    commands *node = (commands *)malloc(sizeof(commands));
    node->prev = NULL;
    node->next = NULL;
    node->token = file;
    return node;
}

instructions* organizer(commands *command){
    if(command == NULL){
        return NULL;
    }
    commands *command_ptr = command;
    instructions *instruct_head = (instructions *)malloc(sizeof(instructions));
    instruct_head->exe_command = NULL;
    instruct_head->arguments = NULL;
    instruct_head->std_in = NULL;
    instruct_head->std_out = NULL;
    instruct_head->next = NULL;
    instructions *instruct_ptr = instruct_head;

    while (command_ptr != NULL)
    {
        if(strcmp(command_ptr->token, "|") != 0){
            if(strcmp(command_ptr->token, "<") == 0){
                if(instruct_ptr->exe_command == NULL){
                    write(STDOUT_FILENO, "Bad Command! Empty file for standard input!\n", 36);
                    free_instruction(instruct_head);
                    return NULL;
                }
                if(instruct_ptr->std_in == NULL){
                    if(command_ptr->next != NULL){
                        if(strcmp(command_ptr->next->token, "|") == 0 ||strcmp(command_ptr->next->token, "<") == 0 ||strcmp(command_ptr->next->token, ">") == 0){
                            write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                            write(STDOUT_FILENO, ": Bad Command! No Argument!\n", 29);
                            free_instruction(instruct_head);
                            return NULL;
                        }else{
                            if(strchr(command_ptr->next->token, '*') == NULL){
                                // if(access(command_ptr->token, F_OK) == 0){
                                //     instruct_ptr->std_in = command_ptr->next->token;
                                //     command_ptr = command_ptr->next;
                                // }else{
                                //     write(STDOUT_FILENO, command_ptr->token,strlen(command_ptr->token));
                                //     write(STDOUT_FILENO, ": Bad Command! File doesn't exist!\n", 36);
                                //     free_instruction(instruct_head);
                                //     return NULL;
                                // }
                                instruct_ptr->std_in = command_ptr->next->token;
                                command_ptr = command_ptr->next;
                            }else{
                                glob_t globbuf;
                                globbuf.gl_offs = 0;
                                int glob_status = glob(command_ptr->next->token, GLOB_DOOFFS, NULL, &globbuf);
                                if(glob_status == 0){
                                    if(globbuf.gl_pathc > 1){
                                        write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                                        write(STDOUT_FILENO, ": Bad Command! Too much argument for redirection!\n", 51);
                                        free_instruction(instruct_head);
                                        globfree(&globbuf);
                                        return NULL;
                                    }
                                }else{
                                    write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                                    write(STDOUT_FILENO, ": Bad Command! No matching files!\n", 35);
                                    free_instruction(instruct_head);
                                    globfree(&globbuf);
                                    return NULL;
                                }
                                
                                instruct_ptr->std_in = command_ptr->next->token;
                                command_ptr = command_ptr->next;
                                globfree(&globbuf);
                            }
                        } 
                    }else{
                        write(STDOUT_FILENO, ": Bad Command! No Argument!\n", 29);
                        free_instruction(instruct_head);
                        return NULL;
                    }
                }else{ //  if(instruct_ptr->std_in != NULL)
                    write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                    write(STDOUT_FILENO, ": Bad Command! Setting std_in Twice!\n", 38);
                    free_instruction(instruct_head);
                    return NULL;
                }
            }else if(strcmp(command_ptr->token, ">") == 0){\
                if(instruct_ptr->exe_command == NULL){
                    write(STDOUT_FILENO, "Bad Command! Empty file to get standard input!\n", 36);
                    free_instruction(instruct_head);
                    return NULL;
                }
                if(instruct_ptr->std_out == NULL){
                    if(command_ptr->next != NULL){
                        if(strcmp(command_ptr->next->token, "|") == 0 || strcmp(command_ptr->next->token, "<") == 0 || strcmp(command_ptr->next->token, ">") == 0){
                            write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                            write(STDOUT_FILENO, ": Bad Command! No Argument!\n", 29);
                            free_instruction(instruct_head);
                            return NULL;
                        }else{
                            if(strchr(command_ptr->next->token, '*') == NULL){
                                // if(access(command_ptr->token, F_OK) == 0){
                                //     instruct_ptr->std_out = command_ptr->next->token;
                                //     command_ptr = command_ptr->next;
                                // }else{
                                //     write(STDOUT_FILENO, command_ptr->next->token,strlen(command_ptr->next->token));
                                //     write(STDOUT_FILENO, ": Bad Command! File doesn't exist!\n", 36);
                                //     free_instruction(instruct_head);
                                //     return NULL;
                                // }
                                instruct_ptr->std_out = command_ptr->next->token;
                                command_ptr = command_ptr->next;
                            }else{
                                glob_t globbuf;
                                globbuf.gl_offs = 0;
                                int glob_status = glob(command_ptr->next->token, GLOB_DOOFFS, NULL, &globbuf);
                                if(glob_status == 0){
                                    if(globbuf.gl_pathc > 1){
                                        write(STDOUT_FILENO, command_ptr->next->token, strlen(command_ptr->next->token));
                                        write(STDOUT_FILENO, ": Bad Command! Too much argument for redirection!\n", 51);
                                        free_instruction(instruct_head);
                                        globfree(&globbuf);
                                        return NULL;
                                    }
                                }else{
                                    write(STDOUT_FILENO, command_ptr->next->token, strlen(command_ptr->next->token));
                                    write(STDOUT_FILENO, ": Bad Command! No matching files!\n", 35);
                                    free_instruction(instruct_head);
                                    globfree(&globbuf);
                                    return NULL;
                                }
                                
                                instruct_ptr->std_out = command_ptr->next->token;
                                command_ptr = command_ptr->next;
                                globfree(&globbuf);
                            }
                        } 
                    }else{
                        write(STDOUT_FILENO, ": Bad Command! No Argument!\n", 29);
                        free_instruction(instruct_head);
                        return NULL;
                    }
                }else{ //  if(instruct_ptr->std_in != NULL)
                    write(STDOUT_FILENO, command_ptr->next->token, strlen(command_ptr->next->token));
                    write(STDOUT_FILENO, ": Bad Command! Setting std_out Twice!\n", 39);
                    free_instruction(instruct_head);
                    return NULL;
                }
            }else{
                if(instruct_ptr->exe_command == NULL){
                    if(strchr(command_ptr->token, '*') == NULL){
                        // if(access(command_ptr->token, F_OK) == 0){
                        //     instruct_ptr->exe_command = command_ptr->token;
                        //     generate_argument_node(instruct_ptr, command_ptr);
                        // }else{
                        //     write(STDOUT_FILENO, command_ptr->token,strlen(command_ptr->token));
                        //     write(STDOUT_FILENO, ": Bad Command! File doesn't exist!\n", 36);
                        //     free_instruction(instruct_head);
                        //     return NULL;
                        // }
                        if(strchr(command_ptr->token, '.') == NULL || (strchr(command_ptr->token, '.') != NULL && strchr(command_ptr->token, '/') != NULL)){
                            instruct_ptr->exe_command = command_ptr->token;
                            generate_argument_node(instruct_ptr, command_ptr);
                        }else{
                            write(STDOUT_FILENO, command_ptr->token, strlen(command_ptr->token));
                            write(STDOUT_FILENO, ": Bad Command! Command need to be an executable file!\n", 55);                          
                            free_instruction(instruct_head);
                            return NULL;
                        }
                    }else{
                        glob_t globbuf;
                        globbuf.gl_offs = 0;
                        if(glob(command_ptr->token, GLOB_DOOFFS, NULL, &globbuf) == 0){
                            if(globbuf.gl_pathc > 1){
                                write(STDOUT_FILENO, command_ptr->token, strlen(command_ptr->token));
                                write(STDOUT_FILENO, ": Bad Command! Too much executable files matches!\n", 51);                          
                                free_instruction(instruct_head);
                                globfree(&globbuf);
                                return NULL;
                            }
                        }else{
                            write(STDOUT_FILENO, command_ptr->token, strlen(command_ptr->token));
                            write(STDOUT_FILENO, ": Bad Command! No matching files!\n", 35);
                            free_instruction(instruct_head);
                            globfree(&globbuf);
                            return NULL;
                        }
                        
                        char *cleaning_string = command_ptr->token;
                        char *replacement = (char*)malloc(sizeof(char) *strlen(globbuf.gl_pathv[0])+1);
                        strcpy(replacement, globbuf.gl_pathv[0]);
                        command_ptr->token = replacement;
                        free(cleaning_string);
                        instruct_ptr->exe_command = command_ptr->token;
                        generate_argument_node(instruct_ptr, command_ptr);
                        globfree(&globbuf);
                    }
                }else{
                    if(strchr(command_ptr->token, '*') == NULL){
                        generate_argument_node(instruct_ptr, command_ptr);
                    }else{
                        commands *head = NULL;
                        commands *end = head;
                        glob_t globbuf;
                        globbuf.gl_offs = 0;
                        if(glob(command_ptr->token, GLOB_DOOFFS, NULL, &globbuf) == 0){
                            for(int index = 0; index < globbuf.gl_pathc; index++){
                                commands* new_node = generate_argument_node_for_wildcard(instruct_ptr, globbuf.gl_pathv[index]);
                                if(head == NULL){
                                    head = new_node;
                                    end = head;
                                }else{
                                    end->next = new_node;
                                    new_node->prev = end;
                                    end = new_node;
                                }
                            }
                        }else{
                            write(STDOUT_FILENO, command_ptr->token, strlen(command_ptr->token));
                            write(STDOUT_FILENO, ": Bad Command! No matching files!\n", 35);
                            free_instruction(instruct_head);
                            return NULL;
                        }
                        commands *prev_node = command_ptr->prev;
                        commands *next_node = command_ptr->next;
                        commands *cleaning_node = command_ptr;
                        if(next_node != NULL){
                            prev_node->next = head; 
                            head->prev = prev_node;
                            end->next = next_node;
                            next_node->prev = end;
                            command_ptr = end;
                        }else{
                            prev_node->next = head; 
                            head->prev = prev_node;
                            command_ptr = end;
                        }

                        free(cleaning_node->token);
                        free(cleaning_node);
                        globfree(&globbuf);
                    }
                    
                    // arguments *argument_node = (arguments *)malloc(sizeof(arguments));
                    // argument_node->arg = command_ptr->token; 
                    // argument_node->next = NULL;
                    // if(instruct_ptr->arguments == NULL){
                    //     instruct_ptr->arguments = argument_node;
                    // }else{
                    //     arguments *end = instruct_ptr->arguments;
                    //     while(end->next != NULL){
                    //         end = end->next;
                    //     }
                    //     end->next = argument_node;
                    // }
                }
            }
        }else{
            if(instruct_ptr->exe_command == NULL){
                write(STDOUT_FILENO, "Bad Command! Empty input for pipe!\n", 36);
                free_instruction(instruct_head);
                return NULL;
            }
            instructions *instruct_node = (instructions *)malloc(sizeof(instructions));
            instruct_node->exe_command = NULL;
            instruct_node->arguments = NULL;
            instruct_node->std_in = NULL;
            instruct_node->std_out = NULL;
            instruct_node->next = NULL;

            instruct_ptr->next = instruct_node;
            instruct_ptr = instruct_ptr->next;
        }
        command_ptr = command_ptr->next;
    }
    return instruct_head;
}

// foo bar < baz | quux *.txt > spam
void print_arguments(arguments *arguments_ptr){
    write(STDOUT_FILENO, "----------",11);
    write(STDOUT_FILENO, "\n",1);
    if(arguments_ptr == NULL){
        write(STDOUT_FILENO, "NULL",5);
        write(STDOUT_FILENO, "\n",1);
    }
    arguments *ptr =  arguments_ptr;
    
    while(ptr != NULL){
        write(STDOUT_FILENO, ptr->arg,strlen(ptr->arg));
        write(STDOUT_FILENO, "\n",1);
        ptr = ptr->next;
    }
    
    write(STDOUT_FILENO, "----------",11);
    write(STDOUT_FILENO, "\n",1);
}

void print_instruction(instructions *instructions_ptr){
    if(instructions_ptr != NULL){
        instructions *ptr = instructions_ptr;
        while(ptr != NULL){
            if(ptr->exe_command == NULL){
                write(STDOUT_FILENO, "NULL",5);
                write(STDOUT_FILENO, "\n",1);
            }else{
                write(STDOUT_FILENO, ptr->exe_command, strlen(ptr->exe_command));
                write(STDOUT_FILENO, "\n",1);
            }
            print_arguments(ptr->arguments);
            if(ptr->std_in == NULL){
                write(STDOUT_FILENO, "NULL",5);
                write(STDOUT_FILENO, "\n",1);
            }else{
                write(STDOUT_FILENO, ptr->std_in, strlen(ptr->std_in));
                write(STDOUT_FILENO, "\n",1);
            }
            if(ptr->std_out == NULL){
                write(STDOUT_FILENO, "NULL",5);
                write(STDOUT_FILENO, "\n",1);
            }else{
                write(STDOUT_FILENO, ptr->std_out, strlen(ptr->std_out));
                write(STDOUT_FILENO, "\n",1);
            }
            ptr = ptr->next;
             write(STDOUT_FILENO, "\n",1);
        }
    }
}

commands *StringTokenization(char *format, int size){
    
    char *str = (char *)malloc(sizeof(char) * (size*2 + 1));
    int count_input = 0;
    int count_rec = 0;
    while(count_input < size){
        if(format[count_input] != '<' && format[count_input] != '>' && format[count_input] != '|'){
            memcpy(&str[count_rec], &format[count_input], sizeof(char));
        }else{
            if(str[count_input+1] != ' '){
                memcpy(&str[count_rec], &format[count_input], sizeof(char));
                count_rec++;
                memcpy(&str[count_rec], " ", sizeof(char));
            }else{
                memcpy(&str[count_rec], &format[count_input], sizeof(char));
            }

        }
        count_input++;
        count_rec++;
    }

    commands *head;
    commands *ptr;
    int first_node = 0;
    int index = 0;
    int pre_index = 0;

    if(str[index] == ' '){
        while(str[index] == ' '){
            index++;
        }
        if(str[index] == '\0' || str[index] == '\n'){
            write(STDOUT_FILENO, "No input!\n",11);
            return NULL;
        }
        pre_index = index;
    }

    while(index < size && str[index] != '\0'){   
        if(str[index] == '\n'){
            int length = index - pre_index;

            commands *new_node = (commands *)malloc(sizeof(commands));
            new_node->prev = NULL;
            new_node->next = NULL;
            new_node->token = (char *)malloc(sizeof(char)*(length+1));

            int copying_count = 0;
            for(int count = pre_index; count < index; count++){
                new_node->token[copying_count] = str[count];
                copying_count++;
            }
            new_node->token[copying_count] = '\0';

            if(first_node == 0){
                head = new_node;
                ptr = head;
                first_node = 1;
            }else{
                ptr->next = new_node;
                new_node->prev = ptr;
            }  
        }else{
            if(str[index] == ' ' || str[index] == '<' || str[index] == '>' || str[index] == '|' ){

                int length = index - pre_index;
            
                // make new node
                commands *new_node = (commands *)malloc(sizeof(commands));
                new_node->prev = NULL;
                new_node->next = NULL;
                new_node->token = (char *)malloc(sizeof(char)*(length+1));

                int copying_count = 0;
                for(int count = pre_index; count < index; count++){
                    new_node->token[copying_count] = str[count];
                    copying_count++;
                }
                new_node->token[copying_count] = '\0';

                if(first_node == 0){
                    head = new_node;
                    ptr = head;
                    first_node = 1;
                }else{
                    ptr->next = new_node;
                    new_node->prev = ptr;
                    ptr = ptr->next;
                }

                if(str[index] == ' '){
                    while(str[index] == ' '){
                        index++;
                    }
                    pre_index = index;
                    // index--;
                    // index++;
                }else{
                    // pre_index = index+1;
                    commands *new_node_other = (commands *)malloc(sizeof(commands));
                    new_node_other->prev = NULL;
                    new_node_other->next = NULL;
                    new_node_other->token = (char *)malloc(sizeof(char)*2);
                    new_node_other->token[0] = str[index];
                    new_node_other->token[1] = '\0';

                    if(first_node == 0){
                        head = new_node_other;
                        ptr = head;
                        first_node = 1;
                    }else{
                        ptr->next = new_node_other;
                        new_node_other->prev = ptr;
                        ptr = ptr->next;
                    }
                    if(str[index+1] == ' '){
                        index = index+1;
                        while(str[index] == ' '){
                            index++;
                        }
                        pre_index = index;
                        index--;
                    }else if(str[index+1] == '<' || str[index+1] == '>' || str[index+1] == '|'){
                        write(STDOUT_FILENO, "< > |", 6);
                        write(STDOUT_FILENO, ": Bad Command! Double Redirection or double pipeing!\n", 54);
                        return NULL;
                    }else{
                        pre_index = pre_index+1;
                    }
                }
            }
        }
        index++;
    }
    free(str);
    return head;
}

commands *StringTokenization_batch(char *format, int size){
    char *str = (char *)malloc(sizeof(char) * (size*2));
    int count_input = 0;
    int count_rec = 0;
    while(count_input < size){
        if(format[count_input] != '<' && format[count_input] != '>' && format[count_input] != '|'){
            str[count_rec] = format[count_input];
        }else{
            if(str[count_input+1] != ' '){
                // memcpy(&str[count_rec], &format[count_input], sizeof(char));
                str[count_rec] = format[count_input];
                count_rec++;
                str[count_rec] = ' ';
            }else{
                str[count_rec] = format[count_input];
            }

        }
        count_input++;
        count_rec++;
    }

    commands *head = NULL;
    commands *ptr;
    int index = 0;
    int pre_index = 0;

    if(str[index] == ' '){
        while(str[index] == ' '){
            index++;
        }
        if(str[index] == '\0' || str[index] == '\n'){
            write(STDOUT_FILENO, "No input!\n",11);
            return NULL;
        }
        pre_index = index;
    }

    while(index < size*2 ){
        if(str[index] == '\0'){
            int length = index - pre_index;

            commands *new_node = (commands *)malloc(sizeof(commands));
            new_node->prev = NULL;
            new_node->next = NULL;
            new_node->token = (char *)malloc(sizeof(char)*(length+1));

            int copying_count = 0;
            for(int count = pre_index; count < index; count++){
                new_node->token[copying_count] = str[count];
                copying_count++;
            }
            new_node->token[copying_count] = '\0';

            if(head == NULL){
                head = new_node;
                ptr = head;
            }else{
                ptr->next = new_node;
                new_node->prev = ptr;
            } 
        }else{
            if(str[index] == ' ' || str[index] == '<' || str[index] == '>' || str[index] == '|' ){
                int length = index - pre_index;

                commands *new_node = (commands *)malloc(sizeof(commands));
                new_node->prev = NULL;
                new_node->next = NULL;
                new_node->token = (char *)malloc(sizeof(char)*(length+1));

                int copying_count = 0;
                for(int count = pre_index; count < index; count++){
                    new_node->token[copying_count] = str[count];
                    copying_count++;
                }
                new_node->token[copying_count] = '\0';

                if(head == NULL){
                    head = new_node;
                    ptr = head;
                }else{
                    ptr->next = new_node;
                    new_node->prev = ptr;
                    ptr = ptr->next;
                }

                if(str[index] == ' '){
                    while(str[index] == ' '){
                        index++;
                    }
                    pre_index = index;
                    
                }else{
                    commands *new_node_other = (commands *)malloc(sizeof(commands));
                    new_node_other->prev = NULL;
                    new_node_other->next = NULL;
                    new_node_other->token = (char *)malloc(sizeof(char)*2);
                    new_node_other->token[0] = str[index];
                    new_node_other->token[1] = '\0';

                    if(head == NULL){
                        head = new_node;
                        ptr = head;
                    }else{
                        ptr->next = new_node;
                        new_node->prev = ptr;
                        ptr = ptr->next;
                    }
                    if(str[index+1] == ' '){
                        index = index+1;
                        while(str[index] == ' '){
                            index++;
                        }
                        pre_index = index;
                        index--;
                    }else if(str[index+1] == '<' || str[index+1] == '>' || str[index+1] == '|'){
                        write(STDOUT_FILENO, "< > |", 6);
                        write(STDOUT_FILENO, ": Bad Command! Double Redirection or double pipeing!\n", 54);
                        return NULL;
                    }else{
                        pre_index = pre_index+1;
                    }

                }
            }
        }
        index++;
    }
    free(str);
    return head;
}

void free_arguments(arguments * arg_ptr){
    // arguments *ptr;
    while(arg_ptr != NULL){
        arguments *ptr = arg_ptr->next;
        free(arg_ptr);
        arg_ptr = ptr;
    }
    free(arg_ptr);
}

void free_instruction(instructions *ins_ptr){
    while(ins_ptr != NULL){
        instructions *temp = ins_ptr->next;
        free_arguments(ins_ptr->arguments);
        free(ins_ptr);
        ins_ptr = temp;
    }
    free(ins_ptr);
}

void free_argument_array(char** argv, int argc){
    for(int index = 0; index < argc; index++){
            free(argv[index]);
    }
    free(argv);
}

int child_process_without_pipe(char *command, arguments *argument, char* stdinput, char* stdoutput){
    arguments *ptr = argument;
    int argc = 0;
    while(ptr != NULL){
        argc++;
        ptr = ptr->next;
    }
    ptr = argument;
    char **argv = (char**)malloc(sizeof(char*)*(argc + 1));
    argv[argc] = NULL;
    int count = 0;
    while(ptr != NULL){
        argv[count] = malloc(sizeof(char) * strlen(ptr->arg) + 1);
        strcpy(argv[count], ptr->arg);
        count++;
        ptr = ptr->next;
    }

    int pid = fork();
    if(pid == -1){
        return 1;
    }
    if(pid == 0){
        if(stdinput != NULL){
            int fdinput = open(stdinput, O_RDONLY);
            if(fdinput == -1){
                perror(stdinput);
                return 1;
            }
            dup2(fdinput, STDIN_FILENO);
            close(fdinput);
        }
        if(stdoutput != NULL){
            int fdoutput = open(stdoutput, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if(fdoutput == -1){
                perror(stdoutput);
                return 1;
            }
            dup2(fdoutput, STDOUT_FILENO);
            close(fdoutput);
        }
        execv(command, argv);
        perror(command);
        return 1;
    }
    int wstatus;
    wait(&wstatus);
    if(WIFEXITED(wstatus)){
        for(int index = 0; index < argc + 1; index++){
            free(argv[index]);
        }
        free(argv);
        return 0;
    }else{
        for(int index = 0; index < argc + 1; index++){
            free(argv[index]);
        }
        free(argv);
        return 1;
    }
} 

int pipe_process(instructions *instruction){
    instructions *instruction_left = instruction;
    instructions *instruction_right = instruction->next;
    // first set of arguments left side of the pipe
    arguments *ptr1 = NULL;

    // second set of arguments right side of the pipe
    arguments *ptr2 = NULL;

    int exit_stutus = 0;

    if(instruction_left->std_out != NULL){
        // stdout from left side not linking to pipe
        exit_stutus = find_file_and_execute(instruction_left);
        // exit_stutus =  child_process_without_pipe(instruction_left->exe_command, instruction_left->arguments, instruction_left->std_in, instruction_left->std_out);
    }else if(instruction_right->std_in != NULL){
        // stdin from right not linking to pipe
        exit_stutus = find_file_and_execute(instruction_right);
        // exit_stutus =  child_process_without_pipe(instruction_right->exe_command, instruction_right->arguments, instruction_right->std_in, instruction_right->std_out);
    }else{ // start piping

        int fd[2];

        int pipe_status = pipe(fd);
        if(pipe_status == -1){
            perror("pipe");
            exit_stutus = 1;
            return exit_stutus;
        }

        int left_pid = fork();
        if(left_pid == -1){
            perror(instruction_left->exe_command);
            exit_stutus = 1;
            return exit_stutus;
        }
        if(left_pid == 0){

            ptr1 = instruction_left->arguments;
            int argc1 = 0;
            while(ptr1 != NULL){
                argc1++;
                ptr1 = ptr1->next;
            }
            ptr1 = instruction_left->arguments;
            char **argv1 = (char**)malloc(sizeof(char*)*(argc1 + 1));
            argv1[argc1] = NULL;
            int count = 0;
            while(ptr1 != NULL){
                argv1[count] = malloc(sizeof(char) * strlen(ptr1->arg) + 1);
                strcpy(argv1[count], ptr1->arg);
                count++;
                ptr1 = ptr1->next;
            }

            if(instruction_left->std_in != NULL){
                int fdinput = open(instruction_left->std_in, O_RDONLY);
                if(fdinput == -1){
                    perror(instruction_left->std_in);
                    exit_stutus = 1;
                    return exit_stutus;
                }
                dup2(fdinput, STDIN_FILENO);
                close(fdinput);
            }

            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);

            execv(instruction_left->exe_command, argv1);
            free_argument_array(argv1, argc1 + 1);
            perror(instruction_left->exe_command);
            exit_stutus = 1;
            return exit_stutus;
        }

        int right_pid = fork();
        if(right_pid == -1){
            perror(instruction_right->exe_command);
            exit_stutus = 1;
            return exit_stutus;
        }
        if(right_pid == 0){

            ptr2 = instruction_right->arguments;
            int argc2 = 0;
            while(ptr2 != NULL){
                argc2++;
                ptr2 = ptr2->next;
            }
            ptr2 = instruction_right->arguments;
            char **argv2 = (char**)malloc(sizeof(char*)*(argc2 + 1));
            argv2[argc2] = NULL;
            int count = 0;
            while(ptr2 != NULL){
                argv2[count] = malloc(sizeof(char) * strlen(ptr2->arg) + 1);
                strcpy(argv2[count], ptr2->arg);
                count++;
                ptr2 = ptr2->next;
            }

            if(instruction_right->std_out != NULL){
                int fdoutput = open(instruction_right->std_out, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if(fdoutput == -1){
                    perror(instruction_right->std_out);
                    return 1;
                }
                dup2(fdoutput, STDOUT_FILENO);
                close(fdoutput);
            }

            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);

            execv(instruction_right->exe_command, argv2);
            free_argument_array(argv2, argc2 + 1);
            perror(instruction_right->exe_command);
            exit_stutus = 1;
            return exit_stutus;
        }

        close(fd[0]);
        close(fd[1]);

        int wstatus;
        for(int index = 0; index < 2; index++){
            int w_id = wait(&wstatus);
            char *child_name = w_id == left_pid ? instruction_left->exe_command : instruction_right->exe_command;
            if(WIFEXITED(wstatus)){
                exit_stutus = 0;
            }else{
                exit_stutus = 1;
                printf("%s : exited abnormally!", child_name);
            }
        }
    }

    return exit_stutus;
}

int find_file_and_execute(instructions *insturction_ptr){
    int exit_status = 0;
    if (strchr(insturction_ptr->exe_command, '.') == NULL && strchr(insturction_ptr->exe_command, '/') == NULL){ // bare name
            // existing commands in usr/bin etc
            // executable in cwd
        char *command_build_in_environment[] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/", ""};
        char **path = (char **)malloc(sizeof(char*)*7);

        for(int index = 0; index < 7; index++){
            path[index] = (char *)malloc(sizeof(char)*(strlen(command_build_in_environment[index])+strlen(insturction_ptr->exe_command)+1));
            memcpy(path[index], command_build_in_environment[index], strlen(command_build_in_environment[index]));
            strcpy(path[index]+strlen(command_build_in_environment[index]), insturction_ptr->exe_command);
        }

        int path_num = 0;
        while(access(path[path_num], F_OK | X_OK) != 0){
            path_num++;
            if(path_num == 7){
                break;
            }
        }

        if(path_num != 7){
            exit_status = child_process_without_pipe(path[path_num], insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
            if(exit_status == 1){
                return exit_status;
            }
        }else{
            exit_status = 1;
            write(STDOUT_FILENO, insturction_ptr->exe_command, strlen(insturction_ptr->exe_command));
            write(STDOUT_FILENO, " : file doesn't exist!\n", 24);
        }

        for(int index = 0; index < 7; index++){
            free(path[index]);
        }
        free(path);
    }else{
        exit_status = child_process_without_pipe(insturction_ptr->exe_command, insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
        if(exit_status == 1){
            return exit_status;
        }
    }
    return exit_status;
}

int execution(instructions *instruction){
    int exit_status = 0;
    if(instruction == NULL){
        exit_status = 1;
        return exit_status;
    }
    instructions *insturction_ptr = instruction;
    
    if(insturction_ptr != NULL && insturction_ptr->next == NULL){
        // build-in
        if(strcmp(insturction_ptr->exe_command, "cd") == 0 || strcmp(insturction_ptr->exe_command, "pwd") == 0 || strcmp(insturction_ptr->exe_command, "exit") == 0){
            if (strcmp(insturction_ptr->exe_command, "cd") == 0)
            {
                if(insturction_ptr->next == NULL){
                    if(insturction_ptr->arguments->next != NULL){
                        if(insturction_ptr->arguments->next->next == NULL){
                            exit_status = cd(insturction_ptr->arguments->next->arg);
                        }else{
                            write(STDOUT_FILENO, "cd: too many arguments", 23);
                            write(STDOUT_FILENO, "\n", 1);
                            exit_status = 1;
                        }
                    }else{
                        char* null_p = NULL;
                        exit_status = cd(null_p);
                    }
                }
            }else if(strcmp(insturction_ptr->exe_command, "pwd") == 0 ){
                if(insturction_ptr->std_out != NULL){
                    int fd = open(insturction_ptr->std_out, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    char *working_dirctory = pwd(REDIRECTION_PWD);
                    write(fd, working_dirctory,strlen(working_dirctory));
                    close(fd);
                    free(working_dirctory);
                }else{
                    char *working_dirctory = pwd(NO_REDIRECTION_PWD);
                    free(working_dirctory);
                }
            }else if(strcmp(insturction_ptr->exe_command, "exit") == 0){
                exit_mysh();
            }   
        }else{
            exit_status = find_file_and_execute(insturction_ptr);
        }
        // if (strchr(insturction_ptr->exe_command, '.') == NULL && strchr(insturction_ptr->exe_command, '/') == NULL){ // bare name
        //     // existing commands in usr/bin etc
        //     // executable in cwd
        //     char *command_build_in_environment[] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/", ""};
        //     char **path = (char **)malloc(sizeof(char*)*7);

        //     for(int index = 0; index < 7; index++){
        //         path[index] = (char *)malloc(sizeof(char)*(strlen(command_build_in_environment[index])+strlen(insturction_ptr->exe_command)+1));
        //         memcpy(path[index], command_build_in_environment[index], strlen(command_build_in_environment[index]));
        //         strcpy(path[index]+strlen(command_build_in_environment[index]), insturction_ptr->exe_command);
        //     }
            
        //     // for(int index = 0; index < 7; index++){
        //     //     printf("%s\n", path[index]);
        //     // }

        //     int path_num = 0;
        //     while(access(path[path_num], F_OK | X_OK) != 0){
        //         path_num++;
        //         if(path_num == 7){
        //             break;
        //         }
        //     }

        //     // printf("%d\n", path_num);

        //     if(path_num != 7){
        //         exit_status = child_process_without_pipe(path[path_num], insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
        //         if(exit_status == 1){
        //             return exit_status;
        //         }
        //     }else{
        //         exit_status = 1;
        //         write(STDOUT_FILENO, insturction_ptr->exe_command, strlen(insturction_ptr->exe_command));
        //         write(STDOUT_FILENO, " : file doesn't exist!\n", 24);
        //     }

        //     for(int index = 0; index < 7; index++){
        //         free(path[index]);
        //     }
        //     free(path);
        // }else{
        //     // char *path_command =  getenv(insturction_ptr->exe_command);
        //     // if(path_command == NULL){
        //     //     exit_status = 1;
        //     //     write(STDOUT_FILENO, insturction_ptr->exe_command, strlen(insturction_ptr->exe_command));
        //     //     write(STDOUT_FILENO, " : file doesn't exist!\n", 24);
        //     // }else{
        //     //     exit_status = child_process_without_pipe(path_command, insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
        //     //     if(exit_status == 1){
        //     //         return exit_status;
        //     //     }
        //     // }
        //     exit_status = child_process_without_pipe(insturction_ptr->exe_command, insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
        //     if(exit_status == 1){
        //         return exit_status;
        //     }
        // }
    }else{
        if(strcmp(insturction_ptr->exe_command, "pwd") == 0 ){
            // write(STDOUT_FILENO, "I am here!", 11);
            insturction_ptr->exe_command = "pwd_pipe";
            insturction_ptr->next->std_in = NULL;
            insturction_ptr->next->std_out = NULL;
            exit_status = pipe_process(insturction_ptr);
        }else if(strcmp(insturction_ptr->exe_command, "exit") == 0){
            execution(insturction_ptr->next);
            exit_mysh();
        }else if(strcmp(insturction_ptr->next->exe_command, "exit") == 0){
            if (strchr(insturction_ptr->exe_command, '.') == NULL && strchr(insturction_ptr->exe_command, '/') == NULL){
                char *command_build_in_environment[] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/", ""};
                char **path = (char **)malloc(sizeof(char*)*7);

                for(int index = 0; index < 7; index++){
                    path[index] = (char *)malloc(sizeof(char)*(strlen(command_build_in_environment[index])+strlen(insturction_ptr->exe_command)+1));
                    memcpy(path[index], command_build_in_environment[index], strlen(command_build_in_environment[index]));
                    strcpy(path[index]+strlen(command_build_in_environment[index]), insturction_ptr->exe_command);
                }

                int path_num = 0;
                while(access(path[path_num], F_OK | X_OK) != 0){
                    path_num++;
                    if(path_num == 7){
                        break;
                    }
                }

                if(path_num != 7){
                    exit_status = child_process_without_pipe(path[path_num], insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
                    if(exit_status == 1){
                        return exit_status;
                    }
                }else{
                    exit_status = 1;
                    write(STDOUT_FILENO, insturction_ptr->exe_command, strlen(insturction_ptr->exe_command));
                    write(STDOUT_FILENO, " : file doesn't exist!\n", 24);
                }

                for(int index = 0; index < 7; index++){
                    free(path[index]);
                }
                free(path);
            }else{
                exit_status = child_process_without_pipe(instruction->exe_command, insturction_ptr->arguments, insturction_ptr->std_in, insturction_ptr->std_out);
            }
            exit_mysh();
        }else if(strcmp(insturction_ptr->exe_command, "cd") == 0 || strcmp(insturction_ptr->next->exe_command, "cd") == 0 ){
            if(strcmp(insturction_ptr->exe_command, "cd") == 0){
                if(insturction_ptr->arguments->next != NULL){
                    if(insturction_ptr->arguments->next->next == NULL){
                        exit_status = cd(insturction_ptr->arguments->next->arg);
                    }else{
                        write(STDOUT_FILENO, "cd: too many arguments", 23);
                        write(STDOUT_FILENO, "\n", 1);
                        exit_status = 1;
                        return exit_status;
                    }
                }else{
                    char* null_p = NULL;
                    exit_status = cd(null_p);
                    if(exit_status == 1){
                        return exit_status;
                    }
                }
                exit_status = find_file_and_execute(insturction_ptr->next);
                if(exit_status == 1){
                    return exit_status;
                }
            }else if(strcmp(insturction_ptr->next->exe_command, "cd") == 0){
                exit_status = find_file_and_execute(insturction_ptr);
                if(exit_status == 1){
                    return exit_status;
                }
                if(insturction_ptr->arguments->next != NULL){
                    if(insturction_ptr->arguments->next->next == NULL){
                        exit_status = cd(insturction_ptr->arguments->next->arg);
                    }else{
                        write(STDOUT_FILENO, "cd: too many arguments", 23);
                        write(STDOUT_FILENO, "\n", 1);
                        exit_status = 1;
                        return exit_status;
                    }
                }else{
                    char* null_p = NULL;
                    exit_status = cd(null_p);
                    if(exit_status == 1){
                        return exit_status;
                    }
                }
            }
        }
        else{
            exit_status = pipe_process(insturction_ptr);
        }
    }
     
    return exit_status;
}

int main(int argc, char *argv[]){
    char *buffer = malloc(sizeof(char)*BUFSIZE);
    long int prev_byte = 0;
    long int curr_buffer_size = BUFSIZE;

    int byte = 1;
    char *prompt_succeed = "mysh> ";
    char *prompt_failed = "!mysh> ";
    int prompt_exit_code = 0;

    // batch mode
    if(argc > 1){
        int fd = open(argv[1], O_RDONLY);
        if(fd == -1){
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }

        while(byte > 0){
            byte = read(fd, buffer, curr_buffer_size);
            if((byte + prev_byte) == curr_buffer_size){
                do{
                    buffer = realloc(buffer, sizeof(char)*(curr_buffer_size+BUFSIZE));
                    curr_buffer_size = curr_buffer_size+BUFSIZE;
                    prev_byte = byte + prev_byte;
                    byte = read(fd, buffer+prev_byte, BUFSIZE);
                    // write(STDOUT_FILENO,&buffer[prev_byte],sizeof(char));
                }while((prev_byte+byte) == curr_buffer_size );
                prev_byte = 0;
            }
        }
        close(fd);

        // for(int index = 0; index < curr_buffer_size; index++){
        //         printf("%c", buffer[index]);
        // }
        // printf("\n");

        lines *head = NULL;
        lines *end = NULL;

        int index = 0;
        int pre_index = 0;
        while(index < curr_buffer_size && buffer[index] != -66){
            if(buffer[index] == '\n'){
                // index = index+1;
                int length = index - pre_index;
                // char *command = (char *)malloc(sizeof(char) * length + 1);

                lines *new_node = (lines *)malloc(sizeof(lines));
                new_node->nextline = NULL;
                new_node->line = (char *)malloc(sizeof(char) * (length + 1));

                int copying_count = 0;
                for(int count = pre_index; count < index; count++){
                    new_node->line[copying_count] = buffer[count];
                    copying_count++;
                }
                new_node->line[length] = '\0';
                // index++;
                pre_index = index;
                pre_index++;

                if(head == NULL){
                    head = new_node;
                    end = head;
                }else{
                    end->nextline = new_node;
                    end = end->nextline;
                }
            }else if(buffer[index] == EOF){
                // index = index+1;
                int length = index - pre_index;
                // char *command = (char *)malloc(sizeof(char) * length + 1);

                lines *new_node = (lines *)malloc(sizeof(lines));
                new_node->nextline = NULL;
                new_node->line = (char *)malloc(sizeof(char) * (length + 1));

                int copying_count = 0;
                for(int count = pre_index; count < index; count++){
                    new_node->line[copying_count] = buffer[count];
                    copying_count++;
                }
                new_node->line[length] = '\0';
                // index++;
                pre_index = index;

                if(head == NULL){
                    head = new_node;
                    end = head;
                }else{
                    end->nextline = new_node;
                    end = end->nextline;
                }
            }
            index++;
        }

        // lines *temp1 = head;
        // while(temp1 != NULL){
        //     printf("%ld", strlen(temp1->line));
        //     printf("%s\n", temp1->line);
            
        //     temp1 = temp1->nextline;
        // }
        // printf("\n");
// /*
        lines *line_ptr = head;
        while(line_ptr != NULL){
            // int str_len = strlen(line_ptr->line);
            // line_ptr->line[str_len] = '\n';
            commands *command_ptr = StringTokenization_batch(line_ptr->line, (strlen(line_ptr->line)+1));

            if(command_ptr != NULL){
                // commands *temp = command_ptr;
                // if(temp != NULL){
                //     while(temp != NULL){
                //         write(STDOUT_FILENO, temp->token, strlen(temp->token));
                //         temp = temp->next;
                //         write(STDOUT_FILENO, "\t", 1);
                //     }
                //     write(STDOUT_FILENO, "\n", 1);
                // }
                prompt_exit_code = 0;
            }else{
                prompt_exit_code = 1;
            }

            instructions * ins_ptr = organizer(command_ptr);
            if(ins_ptr != NULL){
                // print_instruction(ins_ptr);
                prompt_exit_code = 0;
            }else{
                prompt_exit_code = 1;
            }
            prompt_exit_code = execution(ins_ptr);

            free_instruction(ins_ptr);

            
            while(command_ptr != NULL){
                commands *free_ptr = command_ptr->next;
                free(command_ptr->token);
                free(command_ptr);
                command_ptr = free_ptr;
            }
            free(command_ptr);
            line_ptr = line_ptr->nextline;
        }

        while(head != NULL){
            lines *temp = head->nextline;
            free(head->line);
            free(head);
            head = temp;
        }
        free(head);

// */
    }else{ 
        printf("Welcome to my shell!\n");
        while(byte > 0){
            if(prompt_exit_code == 0){
                write(STDOUT_FILENO, prompt_succeed, PROMPTSIZE_EXIT_SUCCESS);
            }else{
                write(STDOUT_FILENO, prompt_failed, PROMPTSIZE_EXIT_FAILURE);
            }

            byte = read(STDIN_FILENO, buffer, curr_buffer_size);
            if((byte + prev_byte) == curr_buffer_size){
                do{
                    buffer = realloc(buffer, sizeof(char)*(curr_buffer_size+BUFSIZE));
                    curr_buffer_size = curr_buffer_size+BUFSIZE;
                    prev_byte = byte + prev_byte;
                    byte = read(STDIN_FILENO, buffer+prev_byte, BUFSIZE);
                    write(STDOUT_FILENO,&buffer[prev_byte+byte-1],sizeof(char));
                }while((byte + prev_byte) == curr_buffer_size && buffer[prev_byte+byte-1] != '\n');
                prev_byte = 0;
            }

            commands *command_ptr = StringTokenization(buffer, curr_buffer_size);

            if(command_ptr != NULL){
                // commands *temp = command_ptr;
                // if(temp != NULL){
                //     prompt_exit_code = 0;
                //     while(temp != NULL){
                //         write(STDOUT_FILENO, temp->token, strlen(temp->token));
                //         temp = temp->next;
                //         write(STDOUT_FILENO, "\t", 1);
                //     }
                //     write(STDOUT_FILENO, "\n", 1);
                // }
                prompt_exit_code = 0;
            }else{
                prompt_exit_code = 1;
            }

            instructions * ins_ptr = organizer(command_ptr);
            if(ins_ptr != NULL){
                // print_instruction(ins_ptr);
                prompt_exit_code = 0;
            }else{
                prompt_exit_code = 1;
            }
            prompt_exit_code = execution(ins_ptr);

            free_instruction(ins_ptr);

            
            while(command_ptr != NULL){
                commands *free_ptr = command_ptr->next;
                free(command_ptr->token);
                free(command_ptr);
                command_ptr = free_ptr;
            }
            free(command_ptr);

            memset(buffer, 0, curr_buffer_size);
        }
    }
   
    free(buffer);
    return EXIT_SUCCESS;
}
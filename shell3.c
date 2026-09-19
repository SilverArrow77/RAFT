#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<fcntl.h>

int main(){
    char *line, *args[100];
    char PATH_VARIABLE[1024] = "/bin:/usr/bin";

    while(1){
        line = readline("prompt> ");
        if(line == NULL){
            break;
        }

        if(strchr(line, '|') != NULL){

            char *commands[100];
            int commandCount = 0;

            commands[commandCount] = strtok(line, "|");

            while(commands[commandCount] != NULL){
                commandCount++;
                commands[commandCount] = strtok(NULL, "|");
            }

            int pipes[100][2];
            int i;

            for(i = 0; i < commandCount - 1; i++){
                pipe(pipes[i]);
            }
            for(i = 0; i < commandCount; i++){

                if(fork() == 0){
                    if(i > 0){
                        close(0);
                        dup2(pipes[i - 1][0], 0);
                    }

                    if(i < commandCount - 1){
                        close(1);
                        dup2(pipes[i][1], 1);
                    }

                    int j;

                    for(j = 0; j < commandCount - 1; j++){
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    int k = 0;
                    args[k] = strtok(commands[i], " ");

                    char *inputFile = NULL;
                    char *outputFile = NULL;

                    while(args[k] != NULL){
                        if(strcmp(args[k], "<") == 0){
                            args[k] = NULL;
                            k++;
                            inputFile = strtok(NULL, " ");
                            continue;
                        }
                        else if(strcmp(args[k], ">") == 0){
                            args[k] = NULL;
                            k++;
                            outputFile = strtok(NULL, " ");
                            continue;
                        }
                        k++;
                        args[k] = strtok(NULL, " ");
                    }

                    if(args[0] == NULL){
                        exit(0);
                    }

                    if(inputFile != NULL){
                        close(0);
                        int fd = open(inputFile, O_RDONLY);

                        if(fd == -1){
                            write(2, "Input file error\n", 17);
                            exit(1);
                        }
                    }

                    if(outputFile != NULL){
                        close(1);
                        int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                        if(fd == -1){
                            write(2, "Output file error\n", 18);
                            exit(1);
                        }
                    }
                    execl(args[0], args[0], args[1], args[2], args[3], args[4], args[5], NULL);
                    char stored[1024];
                    strcpy(stored, PATH_VARIABLE);

                    char *dir = strtok(stored, ":");

                    while(dir != NULL){
                        char full[1024];
                        int j = 0, k = 0;

                        while(dir[j]){
                            full[j] = dir[j];
                            j++;
                        }

                        full[j++] = '/';

                        while(args[0][k]){
                            full[j++] = args[0][k];
                            k++;
                        }

                        full[j] = '\0';

                        execl(full, args[0], args[1], args[2], args[3], args[4], args[5], NULL);

                        dir = strtok(NULL, ":");
                    }

                    write(2, "Command not found\n", 18);
                    exit(1);
                }
            }

            for(i = 0; i < commandCount - 1; i++){
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            for(i = 0; i < commandCount; i++){
                wait(NULL);
            }

            free(line);
            continue;
        }

        int i = 0;
        args[i] = strtok(line, " ");
        char *inputFile = NULL;
        char *outputFile = NULL;

        while(args[i] != NULL){
            if(strcmp(args[i], "<") == 0){
                args[i] = NULL;
                i++;
                inputFile = strtok(NULL, " ");
                continue;
            }
            else if(strcmp(args[i], ">") == 0){
                args[i] = NULL;
                i++;
                outputFile = strtok(NULL, " ");
                continue;
            }
            i++;
            args[i] = strtok(NULL, " ");
        }

        if(args[0] == NULL){
            free(line);
            continue;
        }

        if(strcmp(args[0], "exit") == 0){
            free(line);
            break;
        }

        if(strncmp(args[0], "PATH=", 5) == 0){
            strcpy(PATH_VARIABLE, args[0] + 5);
            free(line);
            continue;
        }

        if(strcmp(args[0], "cd") == 0){
            if(args[1] != NULL){
                if(chdir(args[1]) != 0){
                    write(2, "Incorrect directory\n", 20);
                }
            }
            else{
                write(2, "Directory not specified\n", 24);
            }
            free(line);
            continue;
        }

        if(fork() == 0){

            if(inputFile != NULL){
                close(0);
                int fd = open(inputFile, O_RDONLY);

                if(fd == -1){
                    write(2, "Input file error\n", 17);
                    exit(1);
                }
            }

            if(outputFile != NULL){
                close(1);
                int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if(fd == -1){
                    write(2, "Output file error\n", 18);
                    exit(1);
                }
            }

            execl(args[0], args[0], args[1], args[2], args[3], args[4], args[5], NULL);

            char stored[1024];
            strcpy(stored, PATH_VARIABLE);

            char *dir = strtok(stored, ":");

            while(dir != NULL){
                char full[1024];
                int j = 0, k = 0;

                while(dir[j]){
                    full[j] = dir[j];
                    j++;
                }

                full[j++] = '/';

                while(args[0][k]){
                    full[j++] = args[0][k];
                    k++;
                }

                full[j] = '\0';

                execl(full, args[0], args[1], args[2], args[3], args[4], args[5], NULL);

                dir = strtok(NULL, ":");
            }

            write(2, "Command not found\n", 18);
            exit(1);
        } 
        else {
            wait(NULL);
        }

        free(line);
    }

    return 0;
}
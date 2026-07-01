/*
 * Original upstream simple-c-shell source kept here for reference.
 * It depends on features that are not fully available in the current OS:
 * signals, process groups, termios, dup/dup2, pipe, chdir/getcwd, env, etc.
 * The buildable reduced shell used on this OS is kept below this block.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
// #include <termios.h>
#include "util.h"

#define LIMIT 256
#define MAXLINE 1024


void init(){
        GBSH_PID = getpid();
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);

        if (GBSH_IS_INTERACTIVE) {
            // while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
            //         kill(GBSH_PID, SIGTTIN);

            act_child.sa_handler = signalHandler_child;
            act_int.sa_handler = signalHandler_int;

            sigaction(SIGCHLD, &act_child, 0);
            sigaction(SIGINT, &act_int, 0);

            // setpgid(GBSH_PID, GBSH_PID);
            // GBSH_PGID = getpgrp();
            // if (GBSH_PID != GBSH_PGID) {
            //         printf("Error, the shell is not process group leader");
            //         exit(EXIT_FAILURE);
            // }
            // tcsetpgrp(STDIN_FILENO, GBSH_PGID);
            // tcgetattr(STDIN_FILENO, &GBSH_TMODES);
            currentDirectory = (char*) calloc(1024, sizeof(char));
        } else {
                printf("Could not make the shell interactive.\n");
                exit(EXIT_FAILURE);
        }
}

void welcomeScreen(){
        printf("\n\t============================================\n");
        printf("\t               Simple C Shell\n");
        printf("\t--------------------------------------------\n");
        printf("\t             Licensed under GPLv3:\n");
        printf("\t============================================\n");
        printf("\n\n");
}

void signalHandler_child(int p){
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
    printf("\n");
}

void signalHandler_int(int p){
    if (kill(pid,SIGTERM) == 0){
        printf("\nProcess %d received a SIGINT signal\n",pid);
        no_reprint_prmpt = 1;
    }else{
        printf("\n");
    }
}
#define WHITE(x)  "\033[0m"  x  "\033[0m"
#define BLUE(x)   "\033[34m"  x  "\033[0m"
#define GREEN(x)  "\033[32m"  x  "\033[0m"

void shellPrompt(){
    // char hostn[1024] = "";
    // gethostname(hostn, sizeof(hostn));
    // printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
    // printf("wqqqq@ZZZ %s > ",getcwd(currentDirectory, 1024));

    char buf[256];

    // char hostn[128] = "";

    // gethostname(hostn, sizeof(hostn));

    snprintf(buf,
             sizeof(buf),
             GREEN("wqqqq@ZZZ") ":" BLUE("%s") "$ " ,
             getcwd(currentDirectory, 1024));

    write(STDOUT_FILENO,
          buf,
          strlen(buf));
}

int changeDirectory(char* args[]){
    if (args[1] == NULL) {
        chdir(getenv("HOME"));
        return 1;
    } else {
        if (chdir(args[1]) == -1) {
            printf(" %s: no such directory\n", args[1]);
            return -1;
        }
    }
    return 0;
}

int manageEnviron(char * args[], int option){
    char **env_aux;
    switch(option){
        case 0:
            for(env_aux = environ; *env_aux != 0; env_aux ++){
                printf("%s\n", *env_aux);
            }
            break;
        case 1:
            if((args[1] == NULL) && args[2] == NULL){
                printf("%s","Not enought input arguments\n");
                return -1;
            }

            if(getenv(args[1]) != NULL){
                printf("%s", "The variable has been overwritten\n");
            }else{
                printf("%s", "The variable has been created\n");
            }

            if (args[2] == NULL){
                setenv(args[1], "", 1);
            }else{
                setenv(args[1], args[2], 1);
            }
            break;
        case 2:
            if(args[1] == NULL){
                printf("%s","Not enought input arguments\n");
                return -1;
            }
            if(getenv(args[1]) != NULL){
                unsetenv(args[1]);
                printf("%s", "The variable has been erased\n");
            }else{
                printf("%s", "The variable does not exist\n");
            }
        break;
    }
    return 0;
}

void launchProg(char **args, int background){
    int err = -1;

    if((pid=fork())==-1){
         printf("Child process could not be created\n");
         return;
    }
    if(pid==0){
        signal(SIGINT, SIG_IGN);
        setenv("parent",getcwd(currentDirectory, 1024),1);

        if (execvp(args[0],args)==err){
            printf("Command not found");
            kill(getpid(),SIGTERM);
        }
     }

     if (background == 0){
         waitpid(pid,NULL,0);
     }else{
         printf("Process created with PID: %d\n",pid);
     }
}

void fileIO(char * args[], char* inputFile, char* outputFile, int option){
    int err = -1;
    int fileDescriptor;

    if((pid=fork())==-1){
        printf("Child process could not be created\n");
        return;
    }
    if(pid==0){
        if (option == 0){
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
        }else if (option == 1){
            fileDescriptor = open(inputFile, O_RDONLY, 0600);
            dup2(fileDescriptor, STDIN_FILENO);
            close(fileDescriptor);
            fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fileDescriptor, STDOUT_FILENO);
            close(fileDescriptor);
        }

        setenv("parent",getcwd(currentDirectory, 1024),1);

        if (execvp(args[0],args)==err){
            printf("err");
            kill(getpid(),SIGTERM);
        }
    }
    waitpid(pid,NULL,0);
}
// #define new

void pipeHandler(char *args[]) {
    #define MAX_CMDS 32

    char **cmdv[MAX_CMDS];
    int cmdc = 0;
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    int i;

    /*
     * 解析命令
     *
     * ls | cat | cat
     *
     * =>
     *
     * cmdv[0] -> {"ls", NULL}
     * cmdv[1] -> {"cat", NULL}
     * cmdv[2] -> {"cat", NULL}
     */

    if (args == NULL || args[0] == NULL) {
        return;
    }

    cmdv[cmdc++] = args;
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            if (i == 0 || args[i + 1] == NULL) {
                printf("invalid pipe syntax\n");
                return;
            }
            if (cmdc >= MAX_CMDS) {
                printf("too many piped commands\n");
                return;
            }
            args[i] = NULL;
            cmdv[cmdc++] = &args[i + 1];
        }
    }

    if (cmdc <= 0)
        return;

    /*
     * 创建 pipe
     */

    for (i = 0; i < cmdc - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            printf("pipe failed\n");
            while (--i >= 0) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            return;
        }
    }

    /*
     * fork 所有命令
     */

    for (i = 0; i < cmdc; i++) {

        pids[i] = fork();

        if (pids[i] < 0) {
            printf("fork failed\n");
            return;
        }

        if (pids[i] == 0) {

            /*
             * stdin
             */

            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            /*
             * stdout
             */

            if (i < cmdc - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            /*
             * 关闭所有 pipe fd
             */

            for (int k = 0; k < cmdc - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            execvp(cmdv[i][0], cmdv[i]);

            printf("%s: command not found\n", cmdv[i][0]);
            exit(127);
        }
    }

    /*
     * 父进程关闭全部 pipe
     */

    for (i = 0; i < cmdc - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /*
     * 等待全部子进程
     */

    for (i = 0; i < cmdc; i++) {
        waitpid(pids[i], NULL, 0);
    }
}


int commandHandler(char * args[]){
    int i = 0;
    int j = 0;
    int fileDescriptor;
    int standardOut;
    int aux;
    int background = 0;
    char *args_aux[256];

    while ( args[j] != NULL){
        if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
            break;
        }
        args_aux[j] = args[j];
        j++;
    }

    if(strcmp(args[0],"exit") == 0) exit(0);
    else if (strcmp(args[0],"pwd") == 0){
        if (args[j] != NULL){
            if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
                fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);
                standardOut = dup(STDOUT_FILENO);
                dup2(fileDescriptor, STDOUT_FILENO);
                close(fileDescriptor);
                printf("%s\n", getcwd(currentDirectory, 1024));
                dup2(standardOut, STDOUT_FILENO);
            }
        }else{
            printf("%s\n", getcwd(currentDirectory, 1024));
        }
    }
    else if (strcmp(args[0],"clear") == 0) system("clear");
    else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
    else if (strcmp(args[0],"environ") == 0){
        if (args[j] != NULL){
            if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
                fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);
                standardOut = dup(STDOUT_FILENO);
                dup2(fileDescriptor, STDOUT_FILENO);
                close(fileDescriptor);
                manageEnviron(args,0);
                dup2(standardOut, STDOUT_FILENO);
            }
        }else{
            manageEnviron(args,0);
        }
    }
    else if (strcmp(args[0],"setenv") == 0) manageEnviron(args,1);
    else if (strcmp(args[0],"unsetenv") == 0) manageEnviron(args,2);
    else{
        while (args[i] != NULL && background == 0){
            if (strcmp(args[i],"&") == 0){
                background = 1;
            }else if (strcmp(args[i],"|") == 0){
                pipeHandler(args);
                return 1;
            }else if (strcmp(args[i],"<") == 0){
                aux = i+1;
                if (args[aux] == NULL || args[aux+1] == NULL || args[aux+2] == NULL ){
                    printf("Not enough input arguments\n");
                    return -1;
                }else{
                    if (strcmp(args[aux+1],">") != 0){
                        printf("Usage: Expected '>' and found %s\n",args[aux+1]);
                        return -2;
                    }
                }
                fileIO(args_aux,args[i+1],args[i+3],1);
                return 1;
            }
            else if (strcmp(args[i],">") == 0){
                if (args[i+1] == NULL){
                    printf("Not enough input arguments\n");
                    return -1;
                }
                fileIO(args_aux,NULL,args[i+1],0);
                return 1;
            }
            i++;
        }
        args_aux[i] = NULL;
        launchProg(args_aux,background);
    }
    return 1;
}

static char *shell_readline(char *line, size_t size) {
    size_t len = 0;

    if (!line || size == 0)
        return NULL;

    for (;;) {

        char ch;
        int ret;

        ret = read(STDIN_FILENO, &ch, 1);

        if (ret <= 0) {

            printf("read failed errno=%d\n",errno);

            if (len == 0)
                return NULL;

            break;
        }

        /*
         * echo
         */
        write(STDOUT_FILENO,
              &ch,
              1);

        /*
         * CR/LF 结束
         */
        if (ch == '\r' ||
            ch == '\n') {

            write(STDOUT_FILENO,
                  "\n",
                  1);

            break;
        }

        /*
         * backspace
         */
        if (ch == '\b' || ch == 127) {

            if (len > 0) {

                len--;

                write(STDOUT_FILENO,
                      "\b \b",
                      3);
            }

            continue;
        }

        /*
         * normal char
         */
        if (len + 1 < size) {
            line[len++] = ch;
        }
    }

    line[len] = '\0';

    return line;
}

int main(int argc, char *argv[], char ** envp) {
    char line[MAXLINE];
    char * tokens[LIMIT];
    int numTokens;

    pid = -10;

    init();
    welcomeScreen();
    environ = envp;
    if (getenv("PATH") == NULL || getenv("PATH")[0] == '\0') {
        setenv("PATH", "/bin", 1);
    }
    setenv("shell",getcwd(currentDirectory, 1024),1);
    no_reprint_prmpt = 0;
    while(TRUE){
        if (no_reprint_prmpt == 0) shellPrompt();
        no_reprint_prmpt = 0;
        memset(line, '\0', MAXLINE);
        fgets(line, MAXLINE, stdin);
        // if (!shell_readline(line, MAXLINE))
        //     continue;

        if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
        
        numTokens = 1;
        while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL){
            numTokens++;
        }
        commandHandler(tokens);
    }

    return 0;
}

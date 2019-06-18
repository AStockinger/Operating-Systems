/*******************************************************************
 * Author: Amy Stockinger
 * Date: 5/9/19
 * Program: Small Shell (smallsh.c)
 * Description: simple shell that accepts a general syntax:
 * command [arg1 arg2 ...] [< input_file] [> output_file] [&]
 * where arguments in [] are optional
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAXINPUT 2048
#define ARGUMENTS 512

int bgMode = 0;                // 0 if bg processes allowed, 1 if not

// SIGTSTP handler changes response to '&' in commands
// prints an informative message to the user
void changeMode(){
    if(bgMode == 1){
        char* message = "Exiting foreground-only mode\n";
        write(1, message, 30);              // printf not re-enterant
        fflush(stdout);                     // flush
        bgMode = 0;                         // switch mode
    }
    else if(bgMode == 0){
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(1, message, 50);
        fflush(stdout);                     // flush
        bgMode = 1;                         // switch mode
    }
}

int main(){
    bgMode = 0;                             // 0 if bg processes allowed, 1 if not
    int running = 1;                        // switch that tells input loop to run
    int status = 0;                         // status of process
    char input[MAXINPUT + 1];               // holds user input string
    char* inputFile = NULL;                 // holds in file name
    char* outputFile = NULL;                // holds out file name
    char* devnull = "/dev/null";            // empty input/output
    char* arguments[ARGUMENTS];             // holds arguments extracted from input
    
    long pid = getpid();                    // process id of shell
    char pid_str[10];                       // pid as a string for use in argument array
    snprintf(pid_str, 10, "%d", pid); 

    // catch signal
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_IGN;     // ignore SIGINT
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = changeMode; // catch with a function to change modes
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while(running){
        int bg = 0;                         // background flag, will be 1 if user requests bg process
        char* token;                        // for tokenizing input
        int argCount = 0;                   // counter for argument array
        int r;                              // store results, for loops, or whatever else

        while(argCount <= ARGUMENTS){       // clear all values
            arguments[argCount] = NULL;
            argCount++;
        }
        inputFile = NULL;
        outputFile = NULL;
        argCount = 0;


        /* Get input */
        memset(input, '\0', MAXINPUT + 1);  // reset input buffer
        printf(": ");                       // print colon for prompt
        fflush(stdout);                     // flush prompt
        fgets(input, MAXINPUT, stdin);      // get input from user


        /* Parse input */
        if(input[0] == '#' || input[0] == '\n' || input[0] == '\0'){         // a comment or blank line
            pid_t spawnPid = waitpid(-1, &status, WNOHANG);                  // so check bg process status then reprompt
            while(spawnPid > 0){
                printf("background pid %d is done: ", spawnPid);
                fflush(stdout);                                              // flush
                if (WIFEXITED(status)){                                      // if normal exit,
                    printf("exit value %d\n", WEXITSTATUS(status));          // get status
                    fflush(stdout);                                          // flush
                }
                else{
                    printf("terminated by signal %d\n", WTERMSIG(status));  // else, get termination signal
                    fflush(stdout);                                         // flush
                }
                spawnPid = waitpid(-1, &status, WNOHANG);
            }
            continue;
        }

        char delims[2] = " \n";
        token = strtok(input, delims);
        arguments[argCount] = strdup(token);  // first arg is the command
        argCount++;

        // tokenize input to extract args and look for file redirects
        token = strtok(NULL, delims);
        while(token != NULL){
            if(strcmp(token, "<") == 0){
                token = strtok(NULL, delims); // get next input because it should be a file
                inputFile = strdup(token);
            }
            else if(strcmp(token, ">") == 0){
                token = strtok(NULL, delims); // get next input because it should be a file
                outputFile = strdup(token);
            }
            else if(strcmp(token, "&") == 0){
                token = strtok(NULL, delims); // get next token to see if & is last command
                if(token != NULL){            // if not, skip it
                    continue;
                }
                else if(bgMode == 0){
                    bg = 1;                    // set background switch if permitted
                }
            }
            else{        
                arguments[argCount] = strdup(token);            // add arg to array
                for(r = 0; arguments[argCount][r]; r++){        // check for $$
                    if(arguments[argCount][r] == '$' && arguments[argCount][r + 1] == '$'){
                        arguments[argCount][r] = '\0';          // null terminate the arg right before the first $
                        strcat(arguments[argCount], pid_str);   // append PID instead
                    }
                }
                argCount++;
            }
            token = strtok(NULL, delims);
        }


        /* Execute input */
        // check if command is one of the 3 built-ins
        if(strcmp(arguments[0], "exit") == 0){
            pid_t spawnPid = -5; 
            spawnPid = waitpid(-1, &status, WNOHANG);
            while(spawnPid == 0){
                kill(spawnPid, SIGTERM);                               // kill unfinished child processes
                spawnPid = waitpid(-1, &status, WNOHANG);
            }
            running = 0;                                               // exit by returning 0 from main
        }
        else if(strcmp(arguments[0], "cd") == 0){                      // if change dir, check args
            if(arguments[1] == NULL && argCount == 1){                 // change to HOME if just 'cd'
                chdir(getenv("HOME"));
            }
            else if(argCount == 2){                                     // if ch <dirname> then attempt to change
                if(chdir(arguments[1]) == -1){              
                    printf("Unable to locate specified directory.\n");  // print error if dir not found
                    fflush(stdout);                                     // flush
                }
            }
            else{
                printf("Error: Invalid cd command.\n");                 // otherwise command is invald
            }
        }
        else if(strcmp(arguments[0], "status") == 0){                   // check status
            if (WIFEXITED(status)){                                     // if normal exit,
                printf("exit value %d\n", WEXITSTATUS(status));         // get status
                fflush(stdout);                                         // flush
            }
            else{
                printf("terminated by signal %d\n", WTERMSIG(status));  // else, get termination signal
                fflush(stdout);                                         // flush
            }
        }
        // otherwise execute another command
        else{
            pid_t spawnPid = -5;
            spawnPid = fork();
            switch(spawnPid){
                case -1:
                    perror("Hull Breach!\n");                        // error in fork
                    exit(1);
                    break;
                case 0:
                    if(bg == 0 && bgMode == 0){                      // allow foreground to be interrupted
                        SIGINT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_action, NULL);
                    }
                    if(bg == 1 && bgMode == 0 && inputFile == NULL){ // bg processes input redirected from /dev/null if no file given
                        inputFile = devnull;
                    }
                    if(bg == 1 && bgMode == 0 && outputFile == NULL){// bg processes output redirected to /dev/null if no file given
                        outputFile = devnull;
                    }
                    if(inputFile != NULL){                           // open input file, if any
                        int fileInput = open(inputFile, O_RDONLY);
                        if(fileInput == -1){                         // check for error in file opening
                            perror("open()");
                            exit(1);
                        }
                        int result = dup2(fileInput, 0);             // stdin from input file
                        if(result == -1){                            // check for error
                            perror("dup2");
                            exit(1);
                        }
                        fcntl(fileInput, F_SETFD, FD_CLOEXEC);
                    }
                    if(outputFile != NULL){                                                    // open output file, if any
                        int fileOutput = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644); // truncate or create
                        if(fileOutput == -1){
                            perror("open()");
                            exit(1);
                        }
                        int result = dup2(fileOutput, 1);             // stdout to output file
                        if(result == -1){                             // check for error
                            perror("dup2");
                            exit(1);
                        }
                        fcntl(fileOutput, F_SETFD, FD_CLOEXEC);
                    }
                    if(execvp(arguments[0], arguments) < 0){          // execute and error check
                        perror("exec()");                             // print error
                        exit(1);                                      // exit status 1 if error
                    }
                    break;
                default:
                    if(bg == 1){                                       // if background, don't wait
                        printf("background pid is %d\n", spawnPid);    // print pid
                        fflush(stdout);                                // flush
                    }
                    else{                                              // if foreground,
                        waitpid(spawnPid, &status, 0);                 // wait until completion
                    }
                    break;
            }
        }

        // check background processes
        pid_t spawnPid = waitpid(-1, &status, WNOHANG);                 // check for completed processes
        while(spawnPid > 0){
            printf("background pid %d is done: ", spawnPid);
            fflush(stdout);                                             // flush
            if (WIFEXITED(status)){                                     // if normal exit,
                printf("exit value %d\n", WEXITSTATUS(status));         // get status
                fflush(stdout);                                         // flush
            }
            else{
                printf("terminated by signal %d\n", WTERMSIG(status));  // else, get termination signal
                fflush(stdout);                                         // flush
            }
            spawnPid = waitpid(-1, &status, WNOHANG);
        } 

        // we don't want memory leaks...
        for(r = 0; r < argCount; r++){
            free(arguments[r]);
        }
        free(inputFile);
        free(outputFile);
    }
    return 0;
}
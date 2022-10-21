
/**
 * Name: Chopra Raunak
 * UID: 3035663514
 * Course: COMP3230 - Operating Systems
 *
 * Assignment 1 - Linux Shell
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

// Function to Handle TimeX Command
void timeXHandler(char **args)
{

    int arr = retArgsLength(args);
    if (arr == 0)
    {
        printf("\"timeX\" cannot be a standalone command");
    }
    else
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // child process
            int status_process = execvp(args[0], args);
            if (status_process == -1)
            {
                printf(args[0]);
                printf(": No Such file or directory");
            }
        }
        else
        {
            // parent process
            int status;
            struct rusage rusage;
            int ret = wait4(pid, &status, 0, &rusage);

            printf("(PID)%d  (CMD)%s   (user)%.3f s  (sys)%.3f s", pid, args[0], rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0, rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
        }
    }
    return 0;
}

// To Handle the Exit Command
void exitHandler(char *args)
{
    int arg_count = retArgsLength(args);
    if (arg_count > 1)
    {
        // not a valid case
        printf("\"exit\" with other arguments!!!");
    }
    else if (arg_count == 1)
    {
        // valid case
        pid_t pid = getpid();
        int status;
        struct rusage rusage;
        int ret = wait4(pid, &status, 0, &rusage);
        printf("Terminated!\n");
        exit(1);
    }
}

// To Handle the SIGNINT Signal
void sigint_handler(int signum)
{
    printf(" Receives SIGINT!! IGNORE IT :)");
    printf("\n$$ 3230shell ##  ");
    fflush(stdout);
}

// To obtain stdin from a file
void pipeIn(char *file)
{
    int file_in = open(file, O_RDONLY);
    dup2(file_in, 0);
    close(file_in);
}

// To store stdout in a file from pipe.
void pipeOut(char *file)
{
    int file_out = open(file, O_WRONLY | O_TRUNC | O_CREAT, 0600);
    dup2(file_out, 1);
    close(file_out);
}

// Function to Return the length of the arguments for a command
int retArgsLength(char **args)
{
    int i = 0;
    while (args[i] != NULL)
    {
        i++;
    }
    return i;
}

int wait_check = 1; // flag to determine if process should run in the background

// To Execute a command
void execute(char *args[])
{

    char fileName[] = "/dev/tty";
    pid_t pid;

    // TimeX Command Handler
    if (strcmp(args[0], "timeX") == 0)
    {
        timeXHandler(args + 1);
    }
    // Exit Command Handler
    else if (strcmp(args[0], "exit") == 0)
    {
        exitHandler(args);
    }
    else
    {
        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Fork Failed");
        }
        else if (pid == 0)
        { /* child process */

            int status = execvp(args[0], args);
            if (status == -1)
            {
                printf("'%s'", args[0]);
                printf(": No Such File or Directory");
            }
        }
        else
        {
            if (wait_check)
            {
                waitpid(pid, NULL, 0);
            }
            else
            {
                wait_check = 0;
            }
        }
        pipeIn(fileName);
        pipeOut(fileName);
    }
}

// Converts input command to tokens to handle
char *tokenize(char *command)
{
    int j = 0;
    char *tokenized = (char *)malloc((2048) * sizeof(char));

    int i = 0;
    int len_cmd = strlen(command);
    while (i < strlen(command))
    {
        if (command[i] == '|' && command[i] == '<' && command[i] == '>')
        {
            tokenized[j] = ' ';
            j++;
            tokenized[j] = command[i];
            j++;
            tokenized[j] = ' ';
            j++;
        }
        else
        {
            tokenized[j] = command[i];
            j++;
        }
        i++;
    }
    tokenized[j++] = '\0';

    char *endChar = tokenized + strlen(tokenized) - 1;
    --endChar;
    *(endChar + 1) = '\0';

    return tokenized;
}

// Main Function
int main(void)
{
    signal(SIGINT, sigint_handler);
    char *args[1024]; // command line arguments

    while (1)
    {
        printf("\n$$ 3230shell ## ");
        fflush(stdout);

        char command[1024], *tokens;
        fgets(command, 1024, stdin);
        tokens = tokenize(command);

        char *arg = strtok(tokens, " ");
        int i = 0;
        while (arg)
        {
            if (*arg == '<')
            {
                pipeIn(strtok(NULL, " "));
            }
            else if (*arg == '>')
            {
                pipeOut(strtok(NULL, " "));
            }
            else if (*arg == '|')
            {
                args[i] = NULL;
                int fileDescriptor[2];
                pipe(fileDescriptor);
                dup2(fileDescriptor[1], 1);
                close(fileDescriptor[1]);
                printf("args = %s\n", *args);
                execute(args);
                dup2(fileDescriptor[0], 0);
                close(fileDescriptor[0]);
                i = 0;
            }
            else
            {
                args[i] = arg;
                i++;
            }
            arg = strtok(NULL, " ");
        }
        args[i] = NULL;
        execute(args);
    }
    return 0;
}

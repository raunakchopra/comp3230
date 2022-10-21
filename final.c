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

int should_run = 1;  // flag to determine when to exit program
int should_wait = 1; // flag to determine if process should run in the background

/**
 * Redirects stdin from a file.
 *
 * @param fileName the file to redirect from
 */
void pipeIn(char *fileName)
{
    int in = open(fileName, O_RDONLY);
    dup2(in, 0);
    close(in);
}

/**
 * Redirects stdout to a file.
 *
 * @param fileName the file to redirect to
 */
void pipeOut(char *fileName)
{
    int out = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);
    dup2(out, 1);
    close(out);
}

int retArgsLength(char **args)
{
    int i = 0;
    while (args[i] != NULL)
    {
        i++;
    }
    return i;
}

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
                // handleError();
            }
        }
        else
        {
            // parent process
            int status;
            struct rusage rusage;
            int ret = wait4(pid, &status, 0, &rusage);

            printf("(PID)%d  (CMD)%s   (user)%.3f s  (sys)%.3f s",
                   pid,
                   args[0],
                   rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0,
                   rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
        }
    }
    return;
}

/**
 * Runs a command.
 *
 * @param *args[] the args to run
 */
void run(char *args[])
{
    pid_t pid;
    if (strcmp(args[0], "timeX") == 0)
    {
        timeXHandler(args + 1);
    }
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
        { /* parent process */
            if (should_wait)
            {
                waitpid(pid, NULL, 0);
            }
            else
            {
                should_wait = 0;
            }
        }
        pipeIn("/dev/tty");
        pipeOut("/dev/tty");
    }

}

/**
 * Creates a pipe.
 *
 * @param args [description]
 */
;
void createPipe(char *args[])
{
    int fd[2];
    pipe(fd);

    dup2(fd[1], 1);
    close(fd[1]);

    printf("args = %s\n", *args);

    run(args);

    dup2(fd[0], 0);
    close(fd[0]);
}

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

/**
 * Creates a tokenized form of the input with spaces to separate words.
 *
 * @param  *input the input string
 * @return tokenized the tokenized stirng
 */
char *tokenize(char *input)
{
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((1024 * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++)
    {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|')
        {
            tokenized[j++] = input[i];
        }
        else
        {
            tokenized[j++] = ' ';
            tokenized[j++] = input[i];
            tokenized[j++] = ' ';
        }
    }
    tokenized[j++] = '\0';

    // add null to the end
    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';

    return tokenized;
}

void sigint_handler(int signum)
{
    printf(" Receives SIGINT!! IGNORE IT :)");
    printf("\n$$ 3230shell ##  ");
    fflush(stdout);
    // use default signal handler
    // signal(signum, SIG_DFL);
}

/**
 * Runs a basic shell.
 *
 * @return 0 upon completion
 */
int main(void)
{

    signal(SIGINT, sigint_handler);
    char *args[1024]; // command line arguments

    while (1)
    {
        printf("\n$$ 3230shell ## ");
        fflush(stdout);

        char input[1024];
        fgets(input, 1024, stdin);
        char *tokens;
        tokens = tokenize(input);

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
                createPipe(args);
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

        run(args);
    }
    return 0;
}

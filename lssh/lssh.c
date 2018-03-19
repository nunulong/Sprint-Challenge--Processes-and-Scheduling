#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define PROMPT "lambda-shell$ "

#define MAX_TOKENS 100
#define COMMANDLINE_BUFSIZE 1024
#define DEBUG 1 // Set to 1 to turn on some debugging output, or 0 to turn off

/**
 * Parse the command line.
 *
 * YOU DON'T NEED TO MODIFY THIS!
 * (But you should study it to see how it works)
 *
 * Takes a string like "ls -la .." and breaks it down into an array of pointers
 * to strings like this:
 *
 *   args[0] ---> "ls"
 *   args[1] ---> "-la"
 *   args[2] ---> ".."
 *   args[3] ---> NULL (NULL is a pointer to address 0)
 *
 * @param str {char *} Pointer to the complete command line string.
 * @param args {char **} Pointer to an array of strings. This will hold the result.
 * @param args_count {int *} Pointer to an int that will hold the final args count.
 *
 * @returns A copy of args for convenience.
 */
char **parse_commandline(char *str, char **args, int *args_count)
{
    char *token;

    *args_count = 0;

    token = strtok(str, " \t\n\r");

    while (token != NULL && *args_count < MAX_TOKENS - 1)
    {
        args[(*args_count)++] = token;

        token = strtok(NULL, " \t\n\r");
    }

    args[*args_count] = NULL;

    return args;
}

/**
 * Main
 */
int main(void)
{
    // Holds the command line the user types in
    char commandline[COMMANDLINE_BUFSIZE];

    // Holds the parsed version of the command line
    char *args[MAX_TOKENS];

    // How many command line args the user typed
    int args_count;

    int is_background;

    char *file_name = NULL;

    char **args_pipe;

    // Shell loops forever (until we tell it to exit)
    while (1)
    {
        // Print a prompt
        printf("%s", PROMPT);
        fflush(stdout); // Force the line above to print

        // Read input from keyboard
        fgets(commandline, sizeof commandline, stdin);

        // Wait for any other processes that have ended in the
        // meantime. A more correct solution would be to listen for the
        // SIGCHLD signal and wait for zombies at that point.
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        // Exit the shell on End-Of-File (CRTL-D)
        if (feof(stdin))
        {
            break;
        }

        // Parse input into individual arguments
        parse_commandline(commandline, args, &args_count);

        if (args_count == 0)
        {
            // If the user entered no commands, do nothing
            continue;
        }

        // Exit the shell if args[0] is the built-in "exit" command
        if (strcmp(args[0], "exit") == 0)
        {
            break;
        }

#if DEBUG

        // Some debugging output

        // Print out the parsed command line in args[]
        for (int i = 0; args[i] != NULL; i++)
        {
            printf("%d: '%s'\n", i, args[i]);
        }
#endif

        if (strcmp(args[0], "cd") == 0)
        {
            if (args_count != 2)
            {
                printf("usage: cd dirname\n");
                continue;
            }

            int status = chdir(args[1]);
            if (status == -1)
            {
                perror("chdir");
                continue;
            }

            // successfully change directory
            continue;
        }

        is_background = 0;

        if (strcmp(args[args_count - 1], "&") == 0)
        {
            is_background = 1;
            args[args_count - 1] = NULL;
        }

        if (args_count >= 3 && strcmp(args[args_count - 2], ">") == 0)
        {
            file_name = args[args_count - 1];
            args[args_count - 2] = NULL;
        }

        args_pipe = NULL;
        for (int i = 0; i < args_count; i++)
        {
            if (strcmp(args[i], "|") == 0)
            {
                args_pipe = &(args[i + 1]);
                args[i] = NULL;
            }
        }

        pid_t fk = fork();
        if (fk == -1)
        {
            perror("fork");
            continue;
        }

        if (fk == 0)
        {
            if (file_name != NULL)
            {
                int fd = open(file_name, O_WRONLY | O_CREAT);
                if (fd == -1)
                {
                    perror("redir: open");
                }
                else
                {
                    dup2(fd, 1);
                }
            }

            if (args_pipe != NULL)
            {
                int p[2];
                pipe(p);

                pid_t pipe_child = fork();
                if (pipe_child == 0)
                {
                    dup2(p[0], 0);
                    close(p[1]);

                    execvp(args_pipe[0], args_pipe);
                    perror("exec");
                    exit(1);
                }
                else
                {
                    dup2(p[1], 1);
                    close(p[0]);

                    execvp(args[0], args);
                    perror("exec");
                    exit(1);
                }
            }

            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
        else
        {
            if (!is_background)
            {
                wait(NULL);
            }
        }
    }

    return 0;
}
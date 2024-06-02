#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <glob.h>

#define MAX_COMMAND_LENGTH 1000
#define MAX_ARGS 100

void execute_command(char *argv[]) {
    if (strcmp(argv[0], "cd") == 0) {
        // cd command
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "cd: Number of Parameters Error\n");
        } else if (chdir(argv[1]) != 0) {
            perror("cd");
        }
    } 
    else if (strcmp(argv[0], "pwd") == 0) {
        // pwd command
        char cwd[MAX_COMMAND_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } 
        else {
            perror("pwd");
        }
    } 
    else {
        // external command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        }
        else if (pid == 0) {
            // child progress
        if (execvp(argv[0], argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else {
            // father progress
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void execute_pipe_command(char *args1[], char *args2[]) {
    int pipe_fds[2];
    pid_t pid1, pid2;

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 == 0) {
        // first child
        close(pipe_fds[0]); 
        dup2(pipe_fds[1], STDOUT_FILENO);   //Redirects standard input to the read side of the pipe
        close(pipe_fds[1]);

        execvp(args1[0], args1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        // second child
        close(pipe_fds[1]); 
        dup2(pipe_fds[0], STDIN_FILENO);  //Redirects standard input to the read side of the pipe
        close(pipe_fds[0]);

        execvp(args2[0], args2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // father
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void expand_wildcards(char *arg, char **argv, int *argc) {
  glob_t glob_result;
  int i;

    // GLOB_NOCHECK means that `glob` will return to the original pattern if no matching file is found.
    // GLOB_TILDE allows the wave symbol (`~`) to be used in the pattern to expand to the user's home directory.
  if (glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
    for (i = 0; i < glob_result.gl_pathc && *argc < MAX_ARGS - 1; i++) {
      argv[(*argc)++] = strdup(glob_result.gl_pathv[i]);
    }
  }
  globfree(&glob_result);
}

void parse_and_execute(char *command) {
    char *args[MAX_ARGS];           // Array to store command arguments
    char *args_pipe[MAX_ARGS];      // Array to store arguments after a pipe
    char *token = strtok(command, " "); // Tokenize the command string
    int argc = 0, argc_pipe = 0;    // Argument counters for both command parts
    int pipe_found = 0;             // Flag to check if a pipe is found

    // Loop through tokens while the max argument count is not exceeded
    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, "|") == 0) {
            pipe_found = 1;         // Set the pipe flag if a pipe is found
            break;
        } 
        else if (strchr(token, '*')) {
            // If a wildcard is found, expand it
            expand_wildcards(token, args, &argc);
        } 
        else {
            // Store the argument and increment the argument count
            args[argc++] = strdup(token);
        }
        token = strtok(NULL, " ");  // Get the next token
    }
    args[argc] = NULL;              // Set the last element to NULL for exec

    if (pipe_found) {
        // If a pipe is found, process the arguments after the pipe
        token = strtok(NULL, " ");
        while (token != NULL && argc_pipe < MAX_ARGS - 1) {
            args_pipe[argc_pipe++] = strdup(token); // Store the arguments after the pipe
            token = strtok(NULL, " ");
        }
        args_pipe[argc_pipe] = NULL; // Set the last element to NULL for exec

        execute_pipe_command(args, args_pipe); // Execute the piped command

        // Free dynamically allocated memory for pipe arguments
        for (int i = 0; i < argc_pipe; i++) {
            free(args_pipe[i]);
        }
    } else {
        execute_command(args); // Execute the command if no pipe is found
    }

    // Free dynamically allocated memory for arguments
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }
}

void interactive_mode() {
  char command[MAX_COMMAND_LENGTH];

  printf("welcome to my shell！\n");
  while (1) {
    printf("mysh> ");
    if (!fgets(command, MAX_COMMAND_LENGTH, stdin)) {
      // if get wrong command
      break;
    }

    // remove line break
    command[strcspn(command, "\n")] = 0;

    // if input 'exit', then 'exit'
    if (strcmp(command, "exit") == 0) {
      printf("mysh: exiting\n");
      break;
    }

    parse_and_execute(command);
  }
}

void batch_mode(const char *filename) {
  char command[MAX_COMMAND_LENGTH];
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("open file error");
    exit(EXIT_FAILURE);
  }
  while (fgets(command, MAX_COMMAND_LENGTH, file)) {
    command[strcspn(command, "\n")] = 0;
    parse_and_execute(command);
  }

  fclose(file);
}

int main(int argc, char *argv[]) {
  
  if (argc == 2) {
    // batch_mode
    batch_mode(argv[1]);
  } else {
    // interactive_mode first
    interactive_mode();
  }
  return 0;
}
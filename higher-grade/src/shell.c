#include "parser.h"    // cmd_t, position_t, parse_commands()

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>     //fcntl(), F_GETFL

#define READ  0
#define WRITE 1

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n) {
  for (int i = 0; i < n; i++) {
    printf("==> commands[%d]\n", i);
    printf("  pos = %s\n", position_to_string(commands[i].pos));
    printf("  in  = %d\n", commands[i].in);
    printf("  out = %d\n", commands[i].out);

    print_argv(commands[i].argv);
  }

}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd) {
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error() {
  perror("fork() failed)");
  exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i) {
  if (commands[i].pos == first || commands[i].pos == middle) {
    int fd[2];

    if (pipe(fd) != 0) {
      perror("Failed to create pipe");
      exit(EXIT_FAILURE);
    }
    commands[i].out = fd[WRITE];
    commands[i + 1].in = fd[READ];
  }
    
  pid_t pid;

  switch (pid = fork()) {
    case -1:
      fork_error();
    case 0:
      // Child process after a successful fork().

      if (commands[i].pos == first || commands[i].pos == middle) {
        close(commands[i + 1].in);
        if (dup2(commands[i].out, STDOUT_FILENO) < 0) {
          perror("Failed to redirect stdout");
          exit(EXIT_FAILURE);
        }
      }
      if (commands[i].pos == middle || commands[i].pos == last) {
        if (dup2(commands[i].in, STDIN_FILENO) < 0) {
          perror("Failed to redirect stdin");
          exit(EXIT_FAILURE);
        }
      }

      // Execute the command in the contex of the child process.
      execvp(commands[i].argv[0], commands[i].argv);

      // If execvp() succeeds, this code should never be reached.
      fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
      exit(EXIT_FAILURE);

    default:
      // Parent process after a successful fork().

      if (commands[i].pos == first || commands[i].pos == middle) {
        close(commands[i].out);
      }
      if (commands[i].pos == middle || commands[i].pos == last) {
        close(commands[i].in);
      }
      break;
  }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n) {

  for (int i = 0; i < n; i++) {
    fork_cmd(i);
  }
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char* buffer, size_t size) {
  getline(&buffer, &size, stdin);
  buffer[strlen(buffer)-1] = '\0';
}

/**
 * Make the parents wait for all the child processes.
 */
void wait_for_all_cmds(int n) {
  for (int i = 0; i < n; i++) {
    if (wait(NULL) < 0) {
      perror("Failed to wait");
      exit(EXIT_FAILURE);
    }
  }
}

int main() {
  int n;               // Number of commands in a command pipeline.
  size_t size = 128;   // Max size of a command line string.
  char line[size];     // Buffer for a command line string.


  while(true) {
    printf(" >>> ");

    get_line(line, size);

    n = parse_commands(line, commands);

    fork_commands(n);

    wait_for_all_cmds(n);
  }

  exit(EXIT_SUCCESS);
}

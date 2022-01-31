#include <stdio.h>    // puts(), printf(), perror(), getchar()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <sys/wait.h> // wait()
#include <unistd.h>   // getpid(), getppid(),fork()

#define READ 0
#define WRITE 1

// Close a file descriptor
void close_fd(int fd) {
  if (close(fd) != 0) {
    perror("Failed to close file descriptor");
    exit(EXIT_FAILURE);
  }
}

void child_a(int fd[]) {
  close_fd(fd[WRITE]);

  // Redirect fd[READ] to stdin
  if (dup2(fd[READ], READ) < 0) {
    perror("Failed to redirect stdin");
    exit(EXIT_FAILURE);
  }

  execlp("nl", (char **){"nl", (char *)NULL});
  // We shouldn't be here
  perror("Failed to exec");
  exit(EXIT_FAILURE);
}

void child_b(int fd[]) {
  close_fd(fd[READ]);

  // Redirect fd[WRITE] to stdout
  if (dup2(fd[WRITE], WRITE) < 0) {
    perror("Failed to redirect stdout");
    exit(EXIT_FAILURE);
  }

  execlp("ls", (char **){"ls", "-F", "-1", (char *)NULL});
  // We shouldn't be here
  perror("Failed to exec");
  exit(EXIT_FAILURE);
}

int main(void) {
  int fd[2];

  if (pipe(fd) != 0) {
    perror("Failed to create pipe");
    exit(EXIT_FAILURE);
  }

  pid_t child_a_pid = fork();

  if (child_a_pid < 0) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }
  if (child_a_pid == 0) {
    child_a(fd);
    return 0;
  }
  // We're still in the parent.

  pid_t child_b_pid = fork();

  if (child_b_pid < 0) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }
  if (child_b_pid == 0) {
    child_b(fd);
    return 0;
  }
  // Still in the parent.

  // We don't need any ends of the pipe here so let's close them.
  close_fd(fd[0]);
  close_fd(fd[1]);

  // Wait for the childs to finish.
  if (wait(NULL) < 0) {
    perror("Failed while waiting");
    exit(EXIT_FAILURE);
  }
  if (wait(NULL) < 0) {
    perror("Failed while waiting");
    exit(EXIT_FAILURE);
  }
  return 0;
}

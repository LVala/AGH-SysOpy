#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

void spawn_processes(int n) {
    pid_t pid;
    for (int i=1; i<=n; ++i) {
        if (fork() == 0) {
            pid = getpid();
            printf("Froked process numer %d with PID %d\n", i, (int)pid);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: invalid number of arguments, expected 1\n");
        exit(1);
    }

    int n = strtol(argv[1], NULL, 10);
    if (n == 0) {
        fprintf(stderr, "Error: argument must be a positive number\n");
        exit(1);
    }

    spawn_processes(n);

    return 0;
}
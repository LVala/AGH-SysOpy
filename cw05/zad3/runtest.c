#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void test_many2one(int n) {
    mkfifo("test1.fifo", 0666);
    char buffer[100];

    for (int i=1; i<=5; ++i) {
        if (fork() == 0) {
            sprintf(buffer, "./producer test1.fifo %d example%d.txt %d", i, i, n);
            system(buffer);
            exit(0);
        }
    }
    sprintf(buffer, "./consumer test1.fifo test1_%d_out.txt %d", n, n);
    system(buffer);
}

void test_many2many(int n) {
    mkfifo("test2.fifo", 0666);
    char buffer[100];

    for (int i=1; i<=5; ++i) {
        if (fork() == 0) {
            sprintf(buffer, "./producer test2.fifo %d example%d.txt %d", i, i, n);
            system(buffer);
            exit(0);
        }
    }

    for (int i=1; i<=5; ++i) {
        if (fork() == 0) {
            sprintf(buffer, "./consumer test2.fifo test2_%d_out.txt %d", n, n);
            system(buffer);
            exit(0);
        }
    }
}

void test_one2many(int n) {
    mkfifo("test3.fifo", 0666);
    char buffer[100];

    for (int i=1; i<=5; ++i) {
        if (fork() == 0) {
            sprintf(buffer, "./consumer test3.fifo test3_%d_out.txt %d", n, n);
            system(buffer);
            exit(0);
        }
    }
    
    sprintf(buffer, "./producer test3.fifo %d example%d.txt %d", 1, 1, n);
    system(buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    int n = strtol(argv[1], NULL, 10);
    if (n == 0) {
        fprintf(stderr, "Invalid argument, must be a positive number\n");
        exit(1);
    }

    test_many2one(n);
    test_one2many(n);
    test_many2many(n);

    return 0;
}



#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void signal_handler(int signo) {
    printf("A signal %d has been recieved\n", signo);
}

void do_ignore(int signo) {
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(signo, &act, NULL) == -1) {
        fprintf(stderr, "Signal could not be ignored: %s\n", strerror(errno));
        exit(1);
    }
}

void do_handler(int signo) {
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(signo, &act, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }
}

void do_mask(int signo) {
    sigset_t newmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, signo);
    if (sigprocmask(SIG_BLOCK, &newmask, NULL) < 0) {
        fprintf(stderr, "Signal could not be blocked: %s\n", strerror(errno));
        exit(1);
    }
}

void do_pending(int signo) {
    sigset_t set;
    if (sigpending(&set) == -1) {
        fprintf(stderr, "Pending signals could not be obtained: %s\n", strerror(errno));
        exit(1);
    }
    if (sigismember(&set, signo)) {
        printf("Signal %d is pending\n", signo);
    }
    else {
        printf("Signal %d is not pending\n", signo);
    }
}

void test_signal(char *command, int signo) {
    if (!strcmp(command, "ignore")) {
        do_ignore(signo);
    }
    else if (!strcmp(command, "handler")) {
        do_handler(signo);
    }
    else if (!strcmp(command, "mask") || !strcmp(command, "pending")) {
        do_mask(signo);
    }
    else {
        fprintf(stderr, "Error: invalid argument, do nothing\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments");
        exit(1);
    }
    
    int signo = SIGUSR1;

    if (!strcmp(argv[0], "exec_test")) {
        printf("Exec:\n");
        if (!strcmp(argv[1], "pending")) {
            do_pending(signo);
        }
        else {
            raise(signo);
        }
        printf("Exec: program have been executed correctly\n");
        exit(0);
    }

    test_signal(argv[1], signo);

    printf("Parent:\n");
    raise(signo);

    if (!strcmp(argv[1], "pending"))
        do_pending(signo);

    #ifdef FORK
        pid_t child;
        if ((child = fork()) == -1) {
            fprintf(stderr, "Child process could not be created: %s\n", strerror(errno));
        }
        else if (child == 0) {
            printf("Fork:\n");
            if (!strcmp(argv[1], "pending")) {
                do_pending(signo);
            }
            else {
                raise(signo);
            }
            printf("Fork: program have been executed correctly\n");
            exit(0);
        }
    #endif

    #ifdef EXEC
        execl(argv[0], "exec_test", argv[1], NULL);
    #endif

    printf("Parent: program have been executed correctly\n");
    return 0;
}
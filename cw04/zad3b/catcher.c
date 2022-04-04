#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define KILL 0
#define SIGQUEUE 1
#define SIGRT 2

static int recieved = 0;
static sigset_t blocked;
static int mode = -1;

void handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1 || signo == SIGRTMIN) {
        ++recieved;
        printf("CATCHER: RECIEVED SIGUSR1 OR SIGRTMIN\n");
        kill(info->si_pid, signo);
        sigsuspend(&blocked);
    }
    else if (signo == SIGUSR2 || signo == SIGRTMAX) {
        printf("CATCHER: RECIEVED SIGUSR2 OR SIGRTMAX\n");
        printf("CATCHER: RECIEVED %d SIGNALS\n", recieved);
        kill(info->si_pid, signo);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(1);
    }

    if (!strcmp(argv[1], "kill")) {
        mode = KILL;
    }
    else if (!strcmp(argv[1], "sigqueue")) {
        mode = SIGQUEUE;
    }
    else if (!strcmp(argv[1], "sigrt")) {
        mode = SIGRT;
    }
    else {
        fprintf(stderr, "Error: First arguments is not a recognized command (kill, sigqueue, sigrt)\n");
        exit(1);
    }

    printf("CATCHER: PID %d\n", getpid());

    int sig1 = mode == SIGRT ? SIGRTMIN : SIGUSR1;
    int sig2 = mode == SIGRT ? SIGRTMAX : SIGUSR2;

    sigfillset(&blocked);
    sigdelset(&blocked, sig1);
    sigdelset(&blocked, sig2);
    sigprocmask(SIG_SETMASK, &blocked, NULL);

    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_mask = blocked;
    act.sa_sigaction = handler;
    if (sigaction(sig1, &act, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }
    if (sigaction(sig2, &act, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }

    sigsuspend(&blocked);
    return 0;
}
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define KILL 0
#define SIGQUEUE 1
#define SIGRT 2

static int recieved = 0;
static sigset_t blocked;
static int num_of_sig = 0;

void handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1 || signo == SIGRTMIN) {
        ++recieved;
        printf("SENDER: RECIEVED SIGUSR1 OR SIGRTMIN");
        if (info->si_code == SI_QUEUE) {
            printf(" WITH VALUE: %d", info->si_value.sival_int);
        }
        printf("\n");
        sigsuspend(&blocked);
    }
    else if (signo == SIGUSR2 || signo == SIGRTMAX) {
        printf("SENDER: RECIEVED SIGUSR2 OR SIGRTMAX\n");
        printf("SENDER: RECIEVED %d SIGNALS, EXPECTED %d", recieved, num_of_sig);
        if (info->si_code == SI_QUEUE) {
            printf(", RECIEVED BY CATCHER %d", info->si_value.sival_int);
        }
        printf("\n");
        exit(0);
    }
}

void send_signals(pid_t catcher_pid, int mode) {
    if (mode == SIGQUEUE) {
        union sigval value;
        for (int i=1; i<=num_of_sig; ++i) {
            sigqueue(catcher_pid, SIGUSR1, value);
            printf("SENDER: SEND SIGUSR1 WITH SIGQUEUE\n");
        }
        sigqueue(catcher_pid, SIGUSR2, value);
        printf("SENDER: SEND SIGUSR2 WITH SIGQUEUE\n");
    }
    else {
        int sig1 = mode == KILL ? SIGUSR1 : SIGRTMIN;
        int sig2 = mode == KILL ? SIGUSR2 : SIGRTMAX;
        for (int i=0; i<num_of_sig; ++i) {
            kill(catcher_pid, sig1);
            printf("SENDER: SEND SIGUSR1 OR SIGRTMIX WITH KILL\n");
        }
        kill(catcher_pid, sig2);
        printf("SENDER: SEND SIGUSR2 OR SIGRTMAX WITH KILL\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(1);
    }

    pid_t catcher_pid;
    if ((catcher_pid = strtol(argv[1], NULL, 10)) == 0) {
        fprintf(stderr, "Error: First arguments must be a process PID\n");
        exit(1);
    }
    if (catcher_pid <= 0) {
        fprintf(stderr, "Error: First arguments must be a process PID\n");
        exit(1);
    }

    if ((num_of_sig = strtol(argv[2], NULL, 10)) == 0) {
        fprintf(stderr, "Error: Second arguments must be a positive number\n");
    }
    if (num_of_sig <= 0) {
        fprintf(stderr, "Error: Second arguments must be a positive number\n");
    }

    int mode;
    if (!strcmp(argv[3], "kill")) {
        mode = KILL;
    }
    else if (!strcmp(argv[3], "sigqueue")) {
        mode = SIGQUEUE;
    }
    else if (!strcmp(argv[3], "sigrt")) {
        mode = SIGRT;
    }
    else {
        fprintf(stderr, "Error: Third arguments is not a recognized command (kill, sigqueue, sigrt)\n");
        exit(1);
    }

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

    send_signals(catcher_pid, mode);

    sigsuspend(&blocked);

    return 0;
}
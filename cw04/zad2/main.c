#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

void handler(int sig, siginfo_t *info, void *ucontext) {
    printf("Recieved signal number %d from a process with PID %d. Additional info:\n", info->si_signo, (int)info->si_pid);
    printf("    Real user ID of sending process: %d\n", (int)info->si_uid);
    printf("    User time consumed: %lf\n", (double)info->si_utime/sysconf(_SC_CLK_TCK));
    printf("    An errno value: %d\n\n", (int)info->si_errno);
}


int main(void) {
    // 1st FLAG TEST - SA_SIGINFO
    struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = handler;
    sigemptyset(&act1.sa_mask);

    if (sigaction(SIGUSR1, &act1, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }
    if (sigaction(SIGUSR2, &act1, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }
    if (sigaction(SIGRTMIN, &act1, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }

    printf("1st TEST\n");
    raise(SIGUSR1);
    raise(SIGUSR2);
    raise(SIGRTMIN);

    // 2nd FLAG TEST - SA_NOCLDSTOP
    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
    act2.sa_sigaction = handler;
    sigemptyset(&act2.sa_mask);

    if (sigaction(SIGCHLD, &act2, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }

    printf("2nd TEST\n");
    pid_t child;
    if ((child = fork()) == -1) {
        fprintf(stderr, "Child process could not be created: %s\n", strerror(errno));
    }
    else if (child == 0) {
        raise(SIGSTOP);
    }
    sleep(2);

    // 3rd FLAG TEST - SA_RESETHAND
    struct sigaction act3;
    act3.sa_flags = SA_SIGINFO | SA_RESETHAND;
    act3.sa_sigaction = handler;
    sigemptyset(&act3.sa_mask);

    if (sigaction(SIGUSR1, &act3, NULL) == -1) {
        fprintf(stderr, "Signal could not be handled: %s\n", strerror(errno));
        exit(1);
    }

    printf("3rd TEST\n");
    raise(SIGUSR1);
    raise(SIGUSR1);
    
    return 0;
}
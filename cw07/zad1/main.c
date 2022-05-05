#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "main.h"

int mem_id;
int sem_id;

int cook_number;
pid_t *cooks;
int deliverer_number;
pid_t *deliverers;

void handle_signals(int signo) {
    exit(0);
}

void handle_exit(void) {
    for (int i=0; i<cook_number; ++i) {
        kill(cooks[i], SIGINT);
    }
    for (int i=0; i<deliverer_number; ++i) {
        kill(deliverers[i], SIGINT);
    }

    if (semctl(sem_id, 0, IPC_RMID) == -1) { 
        ERROR(0, 1, "Error: semaphores could not be removed\n");
    }
    if (shmctl(mem_id, IPC_RMID, NULL) == -1) {
        ERROR(0, 1, "Error: shared memory segment could not be removed\n");
    }
}

int main(int argc, char *argv[]) {
    // handle different ways to exit
    atexit(handle_exit);

    struct sigaction act;
    act.sa_handler = handle_signals;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGINT, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n");
    }
    if (sigaction(SIGHUP, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n");
    }
    if (sigaction(SIGQUIT, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n");
    }
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n");
    }

    // handle command line arguments
    if (argc != 3) {
        ERROR(1, 0, "Invalid numver of arguments\n");
    }

    if ((cook_number = strtol(argv[1], NULL, 10)) <= 0) {
        ERROR(1, 1, "Error: invalid first argument\n");
    }
    if ((deliverer_number = strtol(argv[1], NULL, 10)) <= 0) {
        ERROR(1, 1, "Error: invalid second argument\n");
    }

    // semaphores
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = 0;

    key_t sem_key = ftok(OVEN_SEM_PATH, PROJ_ID);
    if (sem_key == -1) {
        ERROR(1, 1, "Error: semaphore key could not be generated\n");
    }
    if ((mem_id = semget(sem_key, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        ERROR(1, 1, "Error: semaphores could not be created\n");
    }
    if (semctl(mem_id, OVEN_SEM, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: oven semaphore could not be reset to 0\n");
    }
    if (semctl(mem_id, TABLE_SEM, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: table semaphore could not be reset to 0\n");
    }

    // shared memory
    key_t mem_key = ftok(MEM_PATH, PROJ_ID);
    if (mem_key == -1) {
        ERROR(1, 1, "Error: oven semaphore key could not be generated\n");
    }
    if ((mem_id = shmget(mem_key, (OVEN_SIZE + TABLE_SIZE)*sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be created\n");
    }

    // create cook processes
    if ((cooks = calloc(cook_number, sizeof(pid_t)))) {
        ERROR(1, 1, "Error: failed to allocate memory\n");
    }
    for (int i=0; i<cook_number; ++i) {
        if ((cooks[i] = fork()) == -1) {
            ERROR(1, 1, "Error: failed to fork\n");
        }
        else if (cooks[i] == 0) {
            execl("./cook", "cook", (char *) NULL);
            ERROR(1, 1, "Error: execution of cook program failed\n");
        }
    }
    
    // create deliverer processes
    if ((deliverers = calloc(deliverer_number, sizeof(pid_t)))) {
        ERROR(1, 1, "Error: failed to allocate memory\n");
    }
    for (int i=0; i<deliverer_number; ++i) {
        if ((deliverers[i] = fork()) == -1) {
            ERROR(1, 1, "Error: failed to fork\n");
        }
        else if (deliverers[i] == 0) {
            execl("./deliverer", "deliverer", (char *) NULL);
            ERROR(1, 1, "Error: execution of deliverer program failed\n");
        }
    }
    
    while (wait(NULL) != -1);

    return 0;
}
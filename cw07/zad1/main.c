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
#include "utils.h"

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
        ERROR(1, 1, "Error: SIGHUP handler could not be set\n");
    }
    if (sigaction(SIGQUIT, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGQUIT handler could not be set\n");
    }
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGTERM handler could not be set\n");
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
    key_t key = ftok(PATH, PROJ_ID);
    if (key == -1) {
        ERROR(1, 1, "Error: semaphore key could not be generated\n");
    }
    if ((sem_id = semget(key, 5, IPC_CREAT | 0666)) == -1) {
        ERROR(1, 1, "Error: semaphores could not be created\n");
    }

    union semun arg;
    arg.val = 1;

    if (semctl(sem_id, OVEN_USED, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: OVEN_USED semaphore could not be reset to 0\n");
    }
    if (semctl(sem_id, TABLE_USED, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: TABLE_USED semaphore could not be reset to 0\n");
    }
    arg.val = 0;
    if (semctl(sem_id, TABLE_READY, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: OVEN_SPACE semaphore could not be reset to 0\n");
    }
    arg.val = OVEN_SIZE;
    if (semctl(sem_id, OVEN_SPACE, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: OVEN_SPACE semaphore could not be reset to 0\n");
    }
    arg.val = TABLE_SIZE;
    if (semctl(sem_id, TABLE_SPACE, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: TABLE_SPACE semaphore could not be reset to 0\n");
    }

    // shared memory
    if ((mem_id = shmget(key, sizeof(struct Oven_table), IPC_CREAT | 0666)) == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be created\n");
    }
    struct Oven_table *oven_table;
    if ((oven_table = shmat(mem_id, NULL, 0)) == (void*) -1) {
        ERROR(1, 1, "Error: shared memory could not be attached\n");
    }

    for (int i=0; i<OVEN_SIZE; ++i)
        oven_table->oven[i] = -1;
    for (int i=0; i<TABLE_SIZE; ++i)
        oven_table->table[i] = -1;
    oven_table->oven_quan = 0;
    oven_table->table_quan = 0;

    if (shmdt(oven_table) == -1) { 
        ERROR(0, 1, "Error: shared memory could not be detached\n");
    }

    // create cook processes
    if ((cooks = calloc(cook_number, sizeof(pid_t))) == NULL) {
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
    if ((deliverers = calloc(deliverer_number, sizeof(pid_t))) == NULL) {
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
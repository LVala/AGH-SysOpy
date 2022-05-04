#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "main.h"

int main(void) {
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = 0;

    // oven semaphore
    key_t oven_sem_key = ftok(OVEN_SEM_PATH, PROJ_ID);
    if (oven_sem_key == -1) {
        ERROR(1, 1, "Error: oven semaphore key could not be generated\n");
    }
    int oven_sem = semget(oven_sem_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (oven_sem == -1) {
        ERROR(1, 1, "Error: oven semaphore could not be created\n");
    }
    if (semctl(oven_sem, 0, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: oven semaphore could not be reset to 0\n");
    }
    // table semaphore
    key_t table_sem_key = ftok(TABLE_SEM_PATH, PROJ_ID);
    if (table_sem_key == -1) {
        ERROR(1, 1, "Error: table semaphore key could not be generated\n");
    }
    int table_sem = semget(table_sem_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (table_sem == -1) {
        ERROR(1, 1, "Error: table semaphore could not be created\n");
    }
    if (semctl(table_sem, 0, SETVAL, arg) == -1) {
        ERROR(1, 1, "Error: table semaphore could not be reset to 0\n");
    }
    // shared memory
    key_t mem_key = ftok(MEM_PATH, PROJ_ID);
    if (mem_key == -1) {
        ERROR(1, 1, "Error: oven semaphore key could not be generated\n");
    }
    int mem = shmget(mem_key, (OVEN_SIZE + TABLE_SIZE)*sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    if (mem == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be created\n");
    }

    // create cook processes
    for (int i=0; i<COOK_NUMBER; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            ERROR(1, 1, "Error: failed to fork\n");
        }
        else if (fork() == 0) {




            exit(0);
        }
    }
    // create deliverer processes
    for (int i=0; i<DELIVERER_NUMBER; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            ERROR(1, 1, "Error: failed to fork\n");
        }
        else if (fork() == 0) {




            exit(0);
        }
    }



    return 0;
}
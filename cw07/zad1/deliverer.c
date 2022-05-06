#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "utils.h"

struct Oven_table *oven_table;
int sem_id;

void handle_signals(int signo) {
    exit(0);
}

void handle_exit(void) {
    if (shmdt(oven_table) == -1) { 
        ERROR(0, 1, "Error: shared memory could not be detached\n");
    }
}

int find_taken(void) {
    for (int i=0; i<TABLE_SIZE; ++i) {
        if (oven_table->table[i] != -1)
            return i;
    }
    return -1;
}

void deliver_pizza(void) {
    struct sembuf sb_table_used;
    sb_table_used.sem_num = TABLE_USED;
    sb_table_used.sem_flg = 0;
    struct sembuf sb_table_space;
    sb_table_space.sem_num = TABLE_SPACE;
    sb_table_space.sem_flg = 0;
    struct sembuf sb_table_ready;
    sb_table_ready.sem_num = TABLE_READY;
    sb_table_ready.sem_flg = 0;

    sb_table_used.sem_op = -1;
    sb_table_ready.sem_op = -1;
    if (semop(sem_id, (struct sembuf[]){sb_table_used, sb_table_ready}, 2) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    int pizza_spot = find_taken();
    int pizza_type = oven_table->table[pizza_spot];
    oven_table->table[pizza_spot] = -1;
    --oven_table->table_quan;
    print_com(getpid(), "Pobieram pizze: %d. Liczba pizz na stole: %d.\n", pizza_type, oven_table->table_quan);

    sb_table_used.sem_op = 1;
    sb_table_space.sem_op = 1;
    if (semop(sem_id, (struct sembuf[]){sb_table_used, sb_table_space}, 2) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }

    randsleep(4, 5);

    print_com(getpid(), "Dostarczam pizze: %d.\n", pizza_type);

    randsleep(4, 5);
}

int main(void) {
    // handle different ways to exit
    atexit(handle_exit);

    struct sigaction act;
    act.sa_handler = handle_signals;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGHUP, &act, NULL) == -1) {
        ERROR(1, 1, "Error: SIGHUP handler could not be set\n");
    }

    // semaphores
    key_t key = ftok(PATH, PROJ_ID);
    if (key == -1) {
        ERROR(1, 1, "Error: semaphore key could not be generated\n");
    }
    if ((sem_id = semget(key, 0, 0)) == -1) {
        ERROR(1, 1, "Error: semaphores could not be created\n");
    }

    // shared memory
    int mem_id;
    if ((mem_id = shmget(key, 0, 0)) == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be created\n");
    }

    if ((oven_table = shmat(mem_id, NULL, 0)) == (void*) -1) {
        ERROR(1, 1, "Error: shared memory could not be attached\n");
    }

    while (1) {
        deliver_pizza();
    }

    return 0;
}
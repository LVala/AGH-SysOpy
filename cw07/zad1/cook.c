#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <signal.h>
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

int find_free(int where) {
    if (where == 0) {  // search in oven
        for (int i=0; i<OVEN_SIZE; ++i) {
            if (oven_table->oven[i] == -1)
                return i;
        }
    }
    else {  // search in table
        for (int i=0; i<TABLE_SIZE; ++i) {
            if (oven_table->table[i] == -1)
                return i;
        }
    }
    return -1;
}

void make_pizza(struct Oven_table *oven_table) {
    struct sembuf sb_oven_used;
    sb_oven_used.sem_num = OVEN_USED;
    sb_oven_used.sem_flg = 0;
    struct sembuf sb_oven_space;
    sb_oven_space.sem_num = OVEN_SPACE;
    sb_oven_space.sem_flg = 0;
    struct sembuf sb_table_used;
    sb_table_used.sem_num = TABLE_USED;
    sb_table_used.sem_flg = 0;
    struct sembuf sb_table_space;
    sb_table_space.sem_num = TABLE_SPACE;
    sb_table_space.sem_flg = 0;
    struct sembuf sb_table_ready;
    sb_table_ready.sem_num = TABLE_READY;
    sb_table_ready.sem_flg = 0;

    int pizza_type = randrange(0, 9);
    print_com(getpid(), "Przygotowuje pizze: %d.\n", pizza_type);
    randsleep(1, 2);

    sb_oven_used.sem_op = -1;
    sb_oven_space.sem_op = -1;
    if (semop(sem_id, (struct sembuf[]){sb_oven_used, sb_oven_space}, 2) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    int pizza_spot = find_free(0);
    oven_table->oven[pizza_spot] = pizza_type;
    ++oven_table->oven_quan;
    print_com(getpid(), "Dodalem pizze: %d. Liczba pizz w piecu: %d.\n", pizza_type, oven_table->oven_quan);
    sb_oven_used.sem_op = 1;
    if (semop(sem_id, &sb_oven_used, 1) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }

    randsleep(4, 5);

    sb_oven_used.sem_op = -1;
    sb_table_used.sem_op = -1;
    sb_table_space.sem_op = -1;
    sb_table_ready.sem_op = 1;
    if (semop(sem_id, (struct sembuf[]){sb_oven_used, sb_table_used, sb_table_space, sb_table_ready}, 4) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    oven_table->oven[pizza_spot] = -1;
    --oven_table->oven_quan;
    pizza_spot = find_free(1);
    oven_table->table[pizza_spot] = pizza_type;
    ++oven_table->table_quan;
    print_com(getpid(), "Wyjmuje pizze: %d. Liczba pizz w piecu: %d. Liczba pizz na stole: %d.\n", pizza_type, oven_table->oven_quan, oven_table->table_quan);

    sb_oven_used.sem_op = 1;
    sb_oven_space.sem_op = 1;
    sb_table_used.sem_op = 1;
    if (semop(sem_id, (struct sembuf[]){sb_oven_used, sb_oven_space, sb_table_used}, 3) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
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

    srand(getpid() * time(NULL));

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
        make_pizza(oven_table);
    }

    return 0;
}
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "utils.h"

struct Oven_table *oven_table;
sem_t *oven_used_sem_id;
sem_t *oven_space_sem_id;
sem_t *table_used_sem_id;
sem_t *table_space_sem_id;
sem_t *table_ready_sem_id;

void handle_signals(int signo) {
    exit(0);
}

void handle_exit(void) {
    sem_close(oven_used_sem_id);
    sem_close(oven_space_sem_id);
    sem_close(table_used_sem_id);
    sem_close(table_space_sem_id);
    sem_close(table_ready_sem_id);

    munmap(oven_table, sizeof(struct Oven_table));
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

void make_pizza(void) {
    int pizza_type = randrange(0, 9);
    print_com(getpid(), "Przygotowuje pizze: %d.\n", pizza_type);
    randsleep(1, 2);

    if ((sem_wait(oven_space_sem_id) == -1) | (sem_wait(oven_used_sem_id) == -1)) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    int pizza_spot = find_free(0);
    oven_table->oven[pizza_spot] = pizza_type;
    ++oven_table->oven_quan;
    print_com(getpid(), "Dodalem pizze: %d. Liczba pizz w piecu: %d.\n", pizza_type, oven_table->oven_quan);

    if (sem_post(oven_used_sem_id) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }

    randsleep(4, 5);

    if (sem_wait(oven_used_sem_id) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    oven_table->oven[pizza_spot] = -1;
    --oven_table->oven_quan;
    if (sem_post(oven_space_sem_id) == -1) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }

    if ((sem_wait(table_space_sem_id) == -1) | (sem_wait(table_used_sem_id) == -1)) {
        ERROR(1, 1, "Error: unable to perform operation on semaphore\n");
    }
    pizza_spot = find_free(1);
    oven_table->table[pizza_spot] = pizza_type;
    ++oven_table->table_quan;
    print_com(getpid(), "Wyjmuje pizze: %d. Liczba pizz w piecu: %d. Liczba pizz na stole: %d.\n", pizza_type, oven_table->oven_quan, oven_table->table_quan);
    
    if ((sem_post(table_ready_sem_id) == -1) | (sem_post(oven_used_sem_id) == -1) | (sem_post(table_used_sem_id) == -1)) {
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
    if ((oven_used_sem_id = sem_open(OVEN_USED, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: OVEN_USED semaphore could not opened\n");
    }
    if ((oven_space_sem_id = sem_open(OVEN_SPACE, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: OVEN_SPACE semaphore could not opened\n");
    }
    if ((table_used_sem_id = sem_open(TABLE_USED, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_USED semaphore could not opened\n");
    }
    if ((table_space_sem_id = sem_open(TABLE_SPACE, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_SPACE semaphore could not opened\n");
    }
    if ((table_ready_sem_id = sem_open(TABLE_READY, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_READY semaphore could not opened\n");
    }

    // shared memory
    int mem_id;
    if ((mem_id = shm_open(MEM, O_RDWR, 0)) == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be opened\n");
    }

    if ((oven_table = mmap(NULL, sizeof(struct Oven_table), PROT_READ | PROT_WRITE, MAP_SHARED, mem_id, 0)) == (void*) -1) {
        ERROR(1, 1, "Error: cook shared memory could not be attached\n");
    }

    while (1) {
        make_pizza();
    }

    return 0;
}
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "utils.h"

sem_t *oven_used_sem_id;
sem_t *oven_space_sem_id;
sem_t *table_used_sem_id;
sem_t *table_space_sem_id;
sem_t *table_ready_sem_id;

int cook_number;
pid_t *cooks;
int deliverer_number;
pid_t *deliverers;

void handle_signals(int signo) {
    exit(0);
}

void handle_exit(void) {
    for (int i=0; i<cook_number; ++i) {  // could also use prctl, Linux only tho
        kill(cooks[i], SIGINT);
    }
    for (int i=0; i<deliverer_number; ++i) {
        kill(deliverers[i], SIGINT);
    }

    sem_close(oven_used_sem_id);
    sem_close(oven_space_sem_id);
    sem_close(table_used_sem_id);
    sem_close(table_space_sem_id);
    sem_close(table_ready_sem_id);

    sem_unlink(OVEN_USED);
    sem_unlink(OVEN_SPACE);
    sem_unlink(TABLE_USED);
    sem_unlink(TABLE_SPACE);
    sem_unlink(TABLE_READY);

    shm_unlink(MEM);
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
    if ((deliverer_number = strtol(argv[2], NULL, 10)) <= 0) {
        ERROR(1, 1, "Error: invalid second argument\n");
    }

    // semaphores
    if ((oven_used_sem_id = sem_open(OVEN_USED, O_CREAT, 0666, 1)) == SEM_FAILED) {
        ERROR(1, 1, "Error: OVEN_USED semaphore could not created\n");
    }
    if ((oven_space_sem_id = sem_open(OVEN_SPACE, O_CREAT, 0666, OVEN_SIZE)) == SEM_FAILED) {
        ERROR(1, 1, "Error: OVEN_SPACE semaphore could not created\n");
    }
    if ((table_used_sem_id = sem_open(TABLE_USED, O_CREAT, 0666, 1)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_USED semaphore could not created\n");
    }
    if ((table_space_sem_id = sem_open(TABLE_SPACE, O_CREAT, 0666, TABLE_SIZE)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_SPACE semaphore could not created\n");
    }
    if ((table_ready_sem_id = sem_open(TABLE_READY, O_CREAT, 0666, 0)) == SEM_FAILED) {
        ERROR(1, 1, "Error: TABLE_READY semaphore could not created\n");
    }

    // shared memory
    int mem_id;
    if ((mem_id = shm_open(MEM, O_RDWR | O_CREAT, 0666)) == -1) {
        ERROR(1, 1, "Error: shared memory segment could not be created\n");
    }
    if (ftruncate(mem_id, sizeof(struct Oven_table)) == -1) {
        ERROR(1, 1, "Error: size of shared memory segment could not be set\n");
    }

    struct Oven_table *oven_table;
    if ((oven_table = mmap(NULL, sizeof(struct Oven_table), PROT_READ | PROT_WRITE, MAP_SHARED, mem_id, 0)) == (void*) -1) {
        ERROR(1, 1, "Error: main shared memory could not be attached\n");
    }

    for (int i=0; i<OVEN_SIZE; ++i)
        oven_table->oven[i] = -1;
    for (int i=0; i<TABLE_SIZE; ++i)
        oven_table->table[i] = -1;
    oven_table->oven_quan = 0;
    oven_table->table_quan = 0;

    if (munmap(oven_table, sizeof(struct Oven_table)) == -1) { 
        ERROR(1, 1, "Error: shared memory could not be detached\n");
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
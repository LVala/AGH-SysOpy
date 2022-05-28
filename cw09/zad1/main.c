#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "utils.h"

pthread_t santa_thread;
pthread_t reindeer_threads[REINDEER_NUM];
pthread_t elf_threads[ELF_NUM];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_elves = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_reindeer = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_santa = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_santa_busy = PTHREAD_COND_INITIALIZER;

int elves;
int reindeers;
pthread_t elves_id[ELVES_REQUIRED];

void *start_routine_santa(void *arg) {
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0) {
        ERROR(1, 1, "Error: asynchronous cancelation of thread could not be set\n");
    }

    int deliveries = 0;
    while (deliveries < SANTA_DELIVERIES) {
        pthread_mutex_lock(&mutex);
        while (reindeers < REINDEER_NUM && elves <  ELVES_REQUIRED) {
            printf("Mikołaj: zasypiam\n");
            pthread_cond_wait(&cond_santa, &mutex);
            printf("Mikołaj: budzę się\n");
        }
        if (reindeers == REINDEER_NUM) {
            printf("Mikołaj: dostarczam zabawki\n");
            pthread_mutex_unlock(&mutex);
            randsleep(2, 4);
            pthread_mutex_lock(&mutex);
            reindeers = 0;
            ++deliveries;
            pthread_cond_broadcast(&cond_reindeer);
        }
        if (deliveries == SANTA_DELIVERIES) {
            printf("Mikołaj: kończę pracę\n");
            pthread_exit((void *) NULL);
        }
        if (elves == ELVES_REQUIRED) {
            pthread_cond_broadcast(&cond_elves);
            printf("Mikołaj: rozwiązuje problemy elfów");
            for (int i=0; i<ELVES_REQUIRED; ++i) {
                printf(" %ld", elves_id[i]);
            }
            printf("\n");

            pthread_mutex_unlock(&mutex);
            randsleep(1, 2);
            pthread_mutex_lock(&mutex);
            elves = 0;
            pthread_cond_broadcast(&cond_elves);
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit((void *) NULL);
}

void *start_routine_reindeer(void *arg) {
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0) {
        ERROR(1, 1, "Error: asynchronous cancelation of thread could not be set\n");
    }

    pthread_t id = pthread_self();

    while (1) {
        randsleep(5, 10);
        pthread_mutex_lock(&mutex);
        printf("Renifer: czeka %d reniferów na Mikołaja, %ld\n", ++(reindeers), id);
        if (reindeers == REINDEER_NUM) {
            printf("Renifer: wybudzam Mikołaja, %ld\n", id);
            pthread_cond_signal(&cond_santa);
        }
        while (reindeers > 0) {
            pthread_cond_wait(&cond_reindeer, &mutex);
        }
        pthread_mutex_unlock(&mutex);
    }
}

void *start_routine_elf(void *arg) {
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0) {
        ERROR(1, 1, "Error: asynchronous cancelation of thread could not be set\n");
    }

    pthread_t id = pthread_self();
    int was_waiting = 0;

    while (1) {
        if (!was_waiting) randsleep(2, 5);
        pthread_mutex_lock(&mutex);
        int index;
        if (elves < ELVES_REQUIRED) {
            index = elves++;
            elves_id[index] = id;
            printf("Elf: czeka %d elfów na Mikołaja, %ld\n", elves, id);
            if (elves == ELVES_REQUIRED) {
                printf("Elf: wybudzam Mikołaja, %ld\n", id);
                pthread_cond_signal(&cond_santa);
            }
            pthread_cond_wait(&cond_elves, &mutex);
            if (pthread_equal(elves_id[index], id)) {
                printf("Elf: Mikołaj rozwiązuje problem, %ld\n", id);
                while (elves > 0) {
                    pthread_cond_wait(&cond_elves, &mutex);
                }
            }
            was_waiting = 0;
        }
        if (elves == ELVES_REQUIRED) {
            printf("Elf: czeka na powrót elfów, %ld\n", id);
            was_waiting = 1;
            while (elves > 0) {
                pthread_cond_wait(&cond_elves, &mutex);
            }
        }
    pthread_mutex_unlock(&mutex);
    }
}

void cancel_threads(int withsanta) {
    if (withsanta){
        pthread_cancel(santa_thread);
        pthread_join(santa_thread, NULL);
    }

    for (int i=0; i<REINDEER_NUM; ++i) {
        pthread_cancel(reindeer_threads[i]);
    }
    for (int i=0; i<ELF_NUM; ++i) {
        pthread_cancel(elf_threads[i]);
    }
}

void handle_exit(void) {
    cancel_threads(1);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_santa);
    pthread_cond_destroy(&cond_elves);
    pthread_cond_destroy(&cond_reindeer);
}

void handle_signals(int signum) {
    exit(0);
}

int main(void) {
    if (atexit(handle_exit) != 0) {
        ERROR(1,1, "Error: atexit function could not be set\n");
    }

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = handle_signals;

    if ((sigaction(SIGHUP, &act, NULL) == -1) | 
        (sigaction(SIGINT, &act, NULL) == -1) |
        (sigaction(SIGQUIT, &act, NULL) == -1) |
        (sigaction(SIGTERM, &act, NULL) == -1)) {
        ERROR(1, 1, "Error: signal handlers could not be set\n");
    }

    if (pthread_create(&santa_thread, NULL, start_routine_santa, (void *) NULL) != 0) {
        ERROR(1, 1, "Error: santa thread could not be created\n");
    }
    for (int i=0; i<REINDEER_NUM; ++i) {
        if (pthread_create(&reindeer_threads[i], NULL, start_routine_reindeer, (void *) NULL) != 0) {
            ERROR(1, 1, "Error: reindeer thread could not be created\n");
        }
    }
    for (int i=0; i<ELF_NUM; ++i) {
        if (pthread_create(&elf_threads[i], NULL, start_routine_elf, (void *) NULL) != 0) {
            ERROR(1, 1, "Error: elf thread could not be created\n");
        }
    }

    pthread_join(santa_thread, NULL);

    cancel_threads(0);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_santa);
    pthread_cond_destroy(&cond_elves);
    pthread_cond_destroy(&cond_reindeer);

    return 0;
}
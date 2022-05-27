#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"

void *start_routine_santa(void *arg) {
    struct Context *context = (struct context *) arg;

    int deliveries = 0;
    while (deliveries < SANTA_DELIVERIES) {
        pthread_mutex_lock(&context->mutex);
        while (context->reindeers < REINDEER_NUM && context->elves <  ELVES_REQUIRED) {
            printf("Mikołaj: zasypiam\n");
            pthread_cond_wait(&context->cond_santa, &context->mutex);
            printf("Mikołaj: budzę się\n");
        }
        // TODO mikołaj zajęty
        if (context->reindeers == REINDEER_NUM) {
            printf("Mikołaj: dostarczam zabawi");
            pthread_mutex_unlock(&context->mutex);
            randsleep(2, 4);
            pthread_mutex_lock(&context->mutex);
            context->reindeers = 0;
            ++deliveries;
            pthread_cond_broadcast(&context->cond_reindeer);
        }
        if (deliveries == SANTA_DELIVERIES) {
            pthread_exit((void *) NULL);
        }
        if (context->elves == ELVES_REQUIRED) {
            printf("Mikołaj: rozwiązuje problemy elfów");
            for (int i=0; i<ELVES_REQUIRED; ++i) {
                printf(" %d", context->elves_id[i]);
            }
            printf("\n");

            pthread_mutex_unlock(&context->mutex);
            randsleep(1, 2);
            pthread_mutex_lock(&context->mutex);
            context->elves = 0;
            pthread_cond_broadcast(&context->cond_elves);
        }
        // TODO mikołaj przestaje być zajęty
        pthread_mutex_unlock(&context->mutex);
    }
    pthread_exit((void *) NULL);
}

void *start_routine_reindeer(void *arg) {
    struct Context *context = (struct context *) arg;
    pthread_t id = pthread_self();

    while (1) {
        randsleep(5, 10);
        pthread_mutex_lock(&context->mutex);
        printf("Renifer: czeka %d reniferów na Mikołaja, %d\n", ++(context->reindeers), id);
        if (context->reindeers == REINDEER_NUM) {
            printf("Renifer: wybudzam Mikołaja, %d\n", id);
            pthread_cond_signal(&context->cond_santa);
            // pthread_cond_broadcast(&context->cond_reindeer); // TODO to mi sie nie podoba
        }
        while (context->reindeers > 0) {
            pthread_cond_wait(&context->cond_reindeer, &context->mutex);
        }
        pthread_mutex_unlock(&context->mutex);
    }
    // pthread_exit((void *) NULL);
}

void *start_routine_elf(void *arg) {
    struct Context *context = (struct context *) arg;
    pthread_t id = pthread_self();

    while (1) {
        randsleep(2, 5);
        pthread_mutex_lock(&context->mutex);
        if (context->elves == ELVES_REQUIRED) {
            printf("Elf: czeka na powrót elfów, %d\n", id);
            while (context->elves > 0) {
                pthread_cond_wait(&context->cond_elves, &context->mutex);
            }
        }



        context->elves_id[context->elves] = id;
        printf("Elf: czeka %d elfów na Mikołaja, %d\n", ++(context->elves), id);
        if (context->elves == ELVES_REQUIRED) {
            printf("Elf: wybudzam Mikołaja, %d\n", id);
            pthread_cond_signal(&context->cond_santa);

        }




        pthread_mutex_unlock(&context->mutex);
    }
    // pthread_exit((void *) NULL);
}

int main(void) {
    // TODO obsluga wyjscia i sygnalow

    struct Context context;

    pthread_t santa_thread;
    pthread_t reindeer_threads[REINDEER_NUM];
    pthread_t elf_threads[ELF_NUM];

    if (pthread_create(&santa_thread, NULL, start_routine_santa, (void *) &context) != 0) {
        ERROR(1, 1, "Error: santa thread could not be created\n");
    }
    for (int i=0; i<REINDEER_NUM; ++i) {
        if (pthread_create(&reindeer_threads[i], NULL, start_routine_reindeer, (void *) &context) != 0) {
            ERROR(1, 1, "Error: reindeer thread could not be created\n");
        }
    }
    for (int i=0; i<ELF_NUM; ++i) {
        if (pthread_create(&elf_threads[i], NULL, start_routine_elf, (void *) &context) != 0) {
            ERROR(1, 1, "Error: elf thread could not be created\n");
        }
    }

    return 0;
}
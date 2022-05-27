#ifndef UTILS_H
#define UTILS_H


#define REINDEER_NUM 9
#define ELF_NUM 10
#define ELVES_REQUIRED 3
#define SANTA_DELIVERIES 3

struct Context {
    pthread_mutex_t mutex;
    pthread_cond_t cond_elves;
    pthread_cond_t cond_reindeer;
    pthread_cond_t cond_santa;
    int elves;
    int elves_id[ELVES_REQUIRED];
    int reindeers;
};

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}

#define randrange(from, to) (rand() % (to + 1 - from) + from)

#define randsleep(from, to) usleep(randrange(from * 1000000, to * 1000000));


#endif
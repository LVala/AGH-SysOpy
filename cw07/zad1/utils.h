#ifndef MAIN_H
#define MAIN_H

#define PIZZA_TYPES 9
#define OVEN_SIZE 5
#define TABLE_SIZE 5

#define OVEN_USED 0
#define OVEN_SPACE 1
#define TABLE_USED 2
#define TABLE_SPACE 3
#define TABLE_READY 4

#define PROJ_ID 1234
#define PATH "/home"

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}

#define print_com(pid, format, ...) {                         \
    char buff[25];                                            \
    time_t now = time(NULL);                                  \
    strftime(buff, 25, "%Y-%m-%d %H:%M:%S", localtime(&now)); \
    printf("(%d %s) ", pid, buff);                            \
    printf(format, ##__VA_ARGS__);                            \
}

#define randrange(from, to) (rand() % (to + 1 - from) + from)

#define randsleep(from, to) usleep(randrange(from * 1000000, to * 1000000));

struct Oven_table {
    int oven[OVEN_SIZE];
    int table[TABLE_SIZE];
    int oven_quan;
    int table_quan;
};

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

#endif
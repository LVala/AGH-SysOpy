#ifndef MAIN_H
#define MAIN_H

#define PIZZA_TYPES 9
#define OVEN_SIZE 5
#define TABLE_SIZE 5

#define OVEN_USED "/oven_used"
#define OVEN_SPACE "/oven_space"
#define TABLE_USED "/table_used"
#define TABLE_SPACE "/table_space"
#define TABLE_READY "/table_ready"

#define MEM "/mem"

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

#endif
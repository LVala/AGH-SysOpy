#ifndef MAIN_H
#define MAIN_H

#define PIZZA_TYPES 9
#define OVEN_SIZE 5
#define TABLE_SIZE 5

#define OVEN_SEM 0
#define TABLE_SEM 1

#define PROJ_ID 1234
#define OVEN_SEM_PATH "/tmp"
#define TABLE_SEM_PATH  "/boot"
#define MEM_PATH "/root"

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}     

#endif
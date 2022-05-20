#ifndef UTILS_H
#define UTILS_H

struct Image {
    int width;
    int height;
    int max_pixel;
    int *values;
};

struct Context {
    struct Image *image;
    int index;
    int threads_num;
};

struct Times {
    struct timespec start;
    struct timespec stop;
};

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}

#define NUMBERS 0
#define BLOCK 1

#define START 0
#define STOP 1

#define MAX_LINE 70

#endif
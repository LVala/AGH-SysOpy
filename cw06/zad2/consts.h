#ifndef CONST_H
#define CONST_H

#define MSG_SIZE 1024  // maximum message size
#define SERVER_PATH "/server"

#define STOP  5
#define LIST  4
#define TOALL 3
#define TOONE 2
#define INIT  1

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}                                                                \

#endif
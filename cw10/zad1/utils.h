#ifndef UTILS_H
#define UTILS_H

#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}

#define MAX_PLAYERS 15
#define MAX_CNAME_LEN 50

enum socket_type {
    LOCAL,
    NET
};

struct message {
    enum type {
        CONNECT,
        
    } type;
    union data {
        char cname[MAX_CNAME_LEN];
    } data;
};


#endif
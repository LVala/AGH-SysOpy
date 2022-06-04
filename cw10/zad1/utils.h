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
#define MAX_EVENTS 5
#define BOARD_SIZE 9
#define SEPARATOR "---+---+---\n"

enum socket_type {
    LOCAL,
    NET
};

struct message {
    enum type {
        CONNECT,
        DISCONNECT,
        PING,
        MOVE,
        BOARD,
        NAME_TAKEN,
        FINISH,
        START,
    } type;
    int id;
    union data {
        char name[MAX_CNAME_LEN];
        char board[BOARD_SIZE];
        char symbol;
        int move;
        char winner;
    } data;
};

struct client {
    char name[MAX_CNAME_LEN];
    int taken;
};


#endif
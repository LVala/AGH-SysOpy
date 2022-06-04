#ifndef UTILS_H
#define UTILS_H


#define ERROR(code, if_errno, format, ...) {                     \
    fprintf(stderr, format, ##__VA_ARGS__);                      \
    if (if_errno)                                                \
        fprintf(stderr, "Error message: %s\n", strerror(errno)); \
    if (code != 0)                                               \
        exit(code);                                              \
}

#define MAX_PLAYERS 12
#define MAX_CNAME_LEN 50
#define MAX_EVENTS 5
#define BOARD_SIZE 9
#define SEPARATOR "---+---+---\n"
#define SYMBOL_1 'X'
#define SYMBOL_2 'O'

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
        TOO_MANY, // TODO
        FINISH,
        START,
    } type;
    int id;
    union data {
        struct cred {
            char name[MAX_CNAME_LEN];
            char symbol;
        } cred;
        char board[BOARD_SIZE];
        int move;
        char winner;
    } data;
};

struct client {
    char name[MAX_CNAME_LEN];
    int taken;
    int opponent;
    int sockfd;
    char *board;
    pthread_mutex_t mutex;
};


#endif
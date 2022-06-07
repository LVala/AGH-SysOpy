#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "utils.h"

int sockfd;
int id;
char symbol;

void creat_local_socket(char *path) {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ERROR(1, 1, "Error: local socket could not be created\n");
    }

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        ERROR(1, 1, "Error: cannot connect to server socket\n");
    }
}

void creat_net_socket(char *address, u_int16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &addr.sin_addr) != 1) {
        ERROR(1, 1 , "Error: could not convert IP address to appropriate format\n");
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ERROR(1, 1, "Error: net socket could not be created\n");
    }

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        ERROR(1, 1, "Error: cannot connect to server socket\n");
    }
}

void print_board(char *board) {
    printf(" %c | %c | %c \n", board[0], board[1], board[2]);
    printf(SEPARATOR);
    printf(" %c | %c | %c \n", board[3], board[4], board[5]);
    printf(SEPARATOR);
    printf(" %c | %c | %c \n", board[6], board[7], board[8]);
}

void handle_connect(struct message *msg) {
    id = msg->id;
    printf("Succesfully connected to the server, waiting for a match...\n");
}

void handle_exit() {
    struct message msg;
    msg.id = id;
    msg.type = DISCONNECT;
    write(sockfd, &msg, sizeof(struct message));
    printf("Exiting...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

void handle_disconnect_signal(int signo) {
    exit(0);
}

void handle_ping() {
    struct message msg;
    msg.id = id;
    msg.type = PING;
    write(sockfd, &msg, sizeof(struct message));
}

void handle_board(struct message *msg) {
    struct message new_msg;
    new_msg.type = MOVE;
    new_msg.id = id;

    print_board(msg->data.board);
    printf("Enter your move (1-9): ");
    scanf("%d", &new_msg.data.move);
    while (new_msg.data.move < 1 || new_msg.data.move > BOARD_SIZE || msg->data.board[new_msg.data.move-1] != ' ') {
        printf("Invalid move, enter valid move (1-9): ");
        scanf("%d", &new_msg.data.move);
    }
    printf("Wait for your turn...\n");
    write(sockfd, &new_msg, sizeof(struct message));
}

void handle_finish(struct message *msg) {
    printf("Game finished: ");
    if (msg->data.winner == 'd') {
        printf("Draw!\n");
    } else if (msg->data.winner == symbol) {
        printf("You won!\n");
    } else {
        printf("You lost!\n");
    }
    exit(0);
}

void handle_start(struct message *msg) {
    symbol = msg->data.cred.symbol;
    printf("Match against %s is starting! Your symbol: %c\nWait for your turn...\n", msg->data.cred.name, symbol);
}

void handle_op_left() {
    printf("Opponent left\n");
    exit(0);
}

void handle_too_many() {
    printf("Too many clients connected to server, cannot create more, exiting...\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        ERROR(1, 0, "Error: invalid number of arguments, expected 3\n");
    }

    atexit(handle_exit);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_disconnect_signal;
    sigaction(SIGINT, &sa, NULL);

    if (strlen(argv[1]) + 1 > MAX_CNAME_LEN) {
        ERROR(1, 0, "Error: client name is too long\n");
    }

    char *name = argv[1];

    if (!strcmp(argv[2], "local")) {
        creat_local_socket(argv[3]);
    }
    else if (!strcmp(argv[2], "net")) {
        char *address = strtok(argv[3], ":");
        int port = strtol(strtok(NULL, ":"), NULL, 10);
        creat_net_socket(address, port);
    }
    else {
        ERROR(1, 0, "Error: invalid second argument, expected either \"local\" or \"net\"\n");
    }

    struct message msg;
    msg.type = CONNECT;
    strcpy(msg.data.cred.name, name);
    write(sockfd, &msg, sizeof(msg));

    while(1) {
        msg.type = -1;
        read(sockfd, &msg, sizeof(msg));
        switch(msg.type) {
            case CONNECT:
                handle_connect(&msg);
                break;
            case DISCONNECT:
                handle_disconnect_signal(0);
                break;
            case PING:
                handle_ping();
                break;
            case BOARD:
                handle_board(&msg);
                break;
            case NAME_TAKEN:
                ERROR(1, 0, "Error: this name is already taken\n");
                break;
            case FINISH:
                handle_finish(&msg);
                break;
            case START:
                handle_start(&msg);
                break;
            case OP_LEFT:
                handle_op_left();
                break;
            case TOO_MANY:
                handle_too_many();
                break;
            default:
                ERROR(1, 0, "Error: recieved message with invalid type\n");   
        }
    }

    return 1;
}
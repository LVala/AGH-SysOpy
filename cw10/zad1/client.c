#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

void handle_disconnect() {
    struct message msg;
    msg.id = id;
    msg.type = DISCONNECT;
    write(sockfd, &msg, sizeof(struct message));
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

void handle_ping() {
    struct message msg;
    msg.id = id;
    msg.type = PING;
    write(sockfd, &msg, sizeof(struct message));
}

void handle_board(struct message *msg) {
    struct message new_msg;
    new_msg.id = id;

    print_board(msg->data.board);
    printff("Enter your move (1-9): ");
    scanf("%d", new_msg.data.move);
    write(sockfd, new_msg, sizeof(struct message));
}

void handle_finish(struct message *msg) {
    printf("Game finished: ");
    if (msg->data.winner == '\0') {
        printf("DRAW!\n");
    } else if (msg->data.winner == symbol) {
        printf("YOU WON!\n");
    } else {
        printf("YOU LOST!\n");
    }
}

void handle_start(struct message *msg) {
    symbol = msg->data.cred.symbol;
    printf("Match against %s is starting! Your symbol: %c\nWait for your turn...\n", msg->data.cred.name, symbol);
    print_board(msg->data.board);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        ERROR(1, 0, "Error: invalid number of arguments, expected 3\n");
    }

    // TODO exit handling

    if (strlen(argv[1]) + 1 > MAX_CNAME_LEN) {
        ERROR(1, 0, "Error: client name is too long\n");
    }

    char *name = argv[1];
    enum socket_type mode;

    if (!strcmp(argv[2], "local")) {
        mode = LOCAL;
        creat_local_socket(argv[3]);
    }
    else if (!strcmp(argv[2], "net")) {
        mode = NET;
        creat_net_socket(strtok(argv[3], ":"), strtol(strtok(NULL, ":"), NULL, 10));
    }
    else {
        ERROR(1, 0, "Error: invalid second argument, expected either \"local\" or \"net\"\n");
    }

    struct message msg;
    msg.type = CONNECT;
    strcpy(msg.data.cred.name, name);
    write(sockfd, &msg, sizeof(msg));

    while(1) {
        read(sockfd, &msg, sizeof(msg));
        switch(msg.type) {
            case CONNECT:
                handle_connect(&msg);
                break;
            case DISCONNECT:
                handle_disconnect();
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
            default:
                ERROR(1, 0, "Error: recieved message with invalid type\n");   
        }
    }

    return 1;
}
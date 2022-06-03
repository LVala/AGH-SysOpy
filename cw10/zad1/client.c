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
    addr.sin_port = 0;  // TODO

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
    strcpy(msg.data.cname, name);
    write(sockfd, &msg, sizeof(msg));

    while(1) {
        read(sockfd, &msg, sizeof(msg));
        switch(msg.type) {
            
        }
    }



    return 0;
}
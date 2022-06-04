#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "utils.h"

int local_sockfd;
int net_sockfd;
int epollfd;
char *path;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct client clients[MAX_PLAYERS];


void creat_clients() {
    for (int i=0; i<MAX_PLAYERS; ++i) {
        clients[i].taken = 0;
        // TODO te mutexy chyba
    }
}

void creat_single_socket(int *sockfd, int domain, struct sockaddr* addr, socklen_t len) {
    if ((*sockfd = socket(domain, SOCK_STREAM, 0)) == -1) {
        ERROR(1, 1, "Error: local socket could not be created\n");
    }
    
    int optval = 1;
    setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval));
    if (bind(*sockfd, addr, len) == -1) {
        ERROR(1, 1, "Error: cannot bind the socket\n");
    }

    listen(*sockfd, MAX_PLAYERS);
}

void creat_sockets(u_int16_t port) {
    struct sockaddr_un addr_un;
    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, path);

    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = INADDR_ANY;

    creat_single_socket(&local_sockfd, AF_UNIX, (struct sockaddr*)&addr_un, sizeof(addr_un));
    printf("Socket listening on UNIX at %s\n", path);
    creat_single_socket(&local_sockfd, AF_INET, (struct sockaddr*)&addr_in, sizeof(addr_in));
    printf("Socket listening on IPV4 on port %d\n", port);
}

void creat_epoll() {
    epollfd = creat_epoll1(0);

    struct epoll_event epoll_event_un;
    epoll_event_un.events = EPOLLIN;
    epoll_event_un.data.fd = local_sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, local_sockfd, &epoll_event_un);

    struct epoll_event epoll_event_in;
    epoll_event_in.events = EPOLLIN;
    epoll_event_in.data.fd = net_sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, net_sockfd, &epoll_event_in);
}

void *start_routine_ping(void *arg) {

}

int check_username(int client_sockfd, char *name) {
    for (int i=0; i<MAX_PLAYERS; ++i) {
        if (clients[i].taken && !strcmp(clients[i].name, name)) {
            struct message msg;
            msg.type = NAME_TAKEN;
            write(client_sockfd, &msg, sizeof(struct message));
            printf("Name already exists, rejecting connection");
            return 1;
        }
    }
    return 0;
}

int register_client(int client_sockfd, struct message *msg) {
    struct message new_msg;

    for (int i=0; i<MAX_PLAYERS; ++i) {
        if (!clients[i].taken) {
            strcpy(clients[i].name, msg->data.cred.name);
            clients[i].taken = 1;
            clients[i].sockfd = client_sockfd;
            clients[i].opponent = -1;

            new_msg.id = i;
            new_msg.type = CONNECT;
            write(client_sockfd, &new_msg, sizeof(struct message));
            return i;
        }
    }
    new_msg.type = TOO_MANY;
    write(client_sockfd, &new_msg, sizeof(struct message));
    return -1;
}

void find_opponent(int id) {
    struct message msg;

    for (int i=0; i<MAX_PLAYERS; ++i) {
        if (i != id && clients[i].taken && clients[i].opponent == -1) {
            clients[id].opponent = i;
            clients[i].opponent = id;

            msg.type = START;
            strcpy(msg.data.cred.name, clients[id].name);
            msg.data.cred.symbol = SYMBOL_1;
            write(clients[i].sockfd, &msg, sizeof(struct message));
            
            strcpy(msg.data.cred.name, clients[i].name);
            msg.data.cred.symbol = SYMBOL_2;
            write(clients[id].sockfd, &msg, sizeof(struct message));

            clients[id].board = malloc(BOARD_SIZE * sizeof(char));  // TODO zwolnic to potem gdzies
            for (int j=0; j<BOARD_SIZE; +j) clients[id].board = ' ';
            clients[i].board = clients[id].board;

            msg.type = BOARD;
            strncpy(msg.data.board, msg.data.board, BOARD_SIZE);

            int who_starts = rand() % 2 == 0 ? id : i;
            write(clients[who_starts].sockfd, &msg, sizeof(struct message));
    
            return;
        }
    }
}

void creat_threads() {
    pthread_t ping_thread;
    pthread_create(&ping_thread, NULL, start_routine_ping, NULL);

    // TODO tu chyba kolejny wątunio

    struct epoll_event events[MAX_EVENTS];
    int nfds;
    struct message msg;

    while(1) {
        if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1) {
            ERROR(1,1, "Error: epoll wait failed\n");
        }
        for (int i=0; i<nfds; ++i) {
            int client_sockfd = accept(events->data.fd, NULL, NULL);
            read(client_sockfd, &msg, sizeof(struct message));

            pthread_mutex_lock(&clients_mutex);
            if (check_username(client_sockfd, msg.data.cred.name)) {
                pthread_mutex_unlock(&clients_mutex);
                continue;
            }
            int new_id;
            if ((new_id = register_client(client_sockfd, &msg)) == -1) {
                pthread_mutex_unlock(&clients_mutex);
                continue;
            }
            find_opponent(new_id);

            pthread_mutex_unlock(&clients_mutex);   
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        ERROR(1, 0, "Error: invalid number of arguments, expected 3\n");
    }

    // exit handling

    u_int16_t port = strtol(argv[1], NULL, 10);
    if (port != 0 && !(port > 1024 && port < 65535)) {
        ERROR(1, 0, "Error: invalid port, expected either 0 or one from range 1024-65535\n");
    }

    path = argv[2];
    // dlugosc sciezki sprawdz

    creat_clients();
    creat_sockets(port);
    creat_epoll();
    creat_threads();

    return 1;
}
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

struct client clients[MAX_PLAYERS];

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

    // initi clients
    creat_sockets(port);
    creat_epoll();
    
    pthread_t ping_thread;
    pthread_create(&ping_thread, NULL, start_routine_ping, NULL);

    struct epoll_event events[MAX_EVENTS];
    int nfds;

    while(1) {
        if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1) {
            ERROR(1,1, "Error: epoll wait failed\n");
        }
        for (int i=0; i<nfds; ++i) {
            int client_sockfd = accept(events->data.fd, NULL, NULL);
        }
    }

    return 1;
}
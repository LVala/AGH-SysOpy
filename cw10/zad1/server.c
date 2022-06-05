#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
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
int clients_epollfd;
char *path;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct client clients[MAX_PLAYERS];


void creat_clients() {
    for (int i=0; i<MAX_PLAYERS; ++i) {
        clients[i].taken = 0;
        clients[i].board = NULL;
        pthread_mutex_init(&clients[i].mutex, NULL);
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
    creat_single_socket(&net_sockfd, AF_INET, (struct sockaddr*)&addr_in, sizeof(addr_in));
    printf("Socket listening on IPV4 on port %d\n", port);
}

void creat_epoll() {
    if ((epollfd = epoll_create1(0)) == -1) {
        ERROR(1, 1, "Error: unable to create epoll\n");
    }

    struct epoll_event epoll_event_un;
    epoll_event_un.events = EPOLLIN;
    epoll_event_un.data.fd = local_sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, local_sockfd, &epoll_event_un);

    struct epoll_event epoll_event_in;
    epoll_event_in.events = EPOLLIN;
    epoll_event_in.data.fd = net_sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, net_sockfd, &epoll_event_in);

    if ((clients_epollfd = epoll_create1(0)) == -1) {
        ERROR(1, 1, "Error: unable to create epoll\n");
    }
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

            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = client_sockfd;
            epoll_ctl(clients_epollfd, EPOLL_CTL_ADD, client_sockfd, &event);

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
            clients[i].symbol = SYMBOL_1;
            write(clients[i].sockfd, &msg, sizeof(struct message));
            
            strcpy(msg.data.cred.name, clients[i].name);
            msg.data.cred.symbol = SYMBOL_2;
            clients[id].symbol = SYMBOL_2;
            write(clients[id].sockfd, &msg, sizeof(struct message));

            clients[id].board = malloc(BOARD_SIZE * sizeof(char));
            for (int j=0; j<BOARD_SIZE; ++j) clients[id].board[j] = ' ';
            clients[i].board = clients[id].board;

            msg.type = BOARD;
            strncpy(msg.data.board, clients[i].board, BOARD_SIZE);

            int who_starts = rand() % 2 == 0 ? id : i;
            write(clients[who_starts].sockfd, &msg, sizeof(struct message));

            return;
        }
    }
}

void handle_disconnect(int id) {
    pthread_mutex_lock(&clients[clients[id].opponent].mutex);
    if (clients[id].opponent != -1) {
        struct message new_msg;
        new_msg.type = OP_LEFT;
        write(clients[clients[id].opponent].sockfd, &new_msg, sizeof(struct message));
        return;
    }
    pthread_mutex_unlock(&clients[clients[id].opponent].mutex);

    struct epoll_event epoll_event_in;
    epoll_event_in.events = EPOLLIN;
    epoll_event_in.data.fd = net_sockfd;
    epoll_ctl(clients_epollfd, EPOLL_CTL_DEL, clients[id].sockfd, &epoll_event_in);

    clients[id].taken = 0;
    clients[id].opponent = -1;
    close(clients[id].sockfd);
    if (clients[id].board != NULL) {
        free(clients[id].board);
        clients[id].board = NULL;
        clients[clients[id].opponent].board = NULL;
    }
}

void *start_routine_ping(void *arg) {
    int efd = epoll_create1(0);
    struct message msg;
    msg.type = PING;
    struct epoll_event event;
    while (1) {
        sleep(4);
        for (int i=0; i<MAX_PLAYERS; ++i) {
            pthread_mutex_lock(&clients[i].mutex);
                if (clients[i].taken) {
                    event.events = EPOLLIN | EPOLLONESHOT;
                    event.data.fd = clients[i].sockfd;
                    epoll_ctl(efd, EPOLL_CTL_ADD, clients[i].sockfd, &event);
                    write(clients[i].sockfd, &msg, sizeof(struct message));
                    printf("Pinged client %d\n", clients[i].sockfd);
                    int ndfs = epoll_wait(efd, &event, 1, TIMEOUT);
                    printf("Recieved ping back from %d\n", clients[i].sockfd);
                    if (ndfs == -1) {
                        printf("Client with descriptor %d go timed out\n", clients[i].sockfd);
                        handle_disconnect(i);
                    }
                }
            pthread_mutex_unlock(&clients[i].mutex);
        }
    } 
    pthread_exit((void *)NULL);
}

char check_line(char f1, char f2, char f3) {
    return f1 != ' ' && f1 == f2 && f2 == f3 ? f1 : '\0';
}

char check_game_status(char *board) {
    char winner = '\0';

    int opts[8][3] = {{0,1,2}, {3,4,5}, {6,7,8}, {0,3,6}, {1,4,7}, {2,5,8}, {0,4,8}, {2,4,6}};

    for (int i=0; i<8; ++i) {
        if ((winner = check_line(board[opts[i][0]], board[opts[i][1]], board[opts[i][2]])) != '\0') {
            return winner;
        }
    }

    for (int i=0; i<BOARD_SIZE; ++i) {
        if (board[i] == ' ') {
            return '\0';
        }
    }

    return 'd';
}

void handle_move(struct message *msg) {
    if (clients[msg->id].opponent == -1) {
        ERROR(1, 0, "Error: server recieved invalid message\n");
    }

    pthread_mutex_lock(&clients[clients[msg->id].opponent].mutex);

    if (clients[clients[msg->id].opponent].taken == 0) {
        struct message new_msg;
        new_msg.type = OP_LEFT;
        write(clients[msg->id].sockfd, &new_msg, sizeof(struct message));
        pthread_mutex_unlock(&clients[clients[msg->id].opponent].mutex);
        return;
    }

    struct message new_msg;
    clients[msg->id].board[msg->data.move-1] = clients[msg->id].symbol;
    int finish;
    if ((finish = check_game_status(clients[msg->id].board)) != 0) {
        new_msg.type = FINISH;
        new_msg.data.winner = finish;
        write(clients[clients[msg->id].opponent].sockfd, &new_msg, sizeof(struct message));
        write(clients[msg->id].sockfd, &new_msg, sizeof(struct message));
    }
    else {
        new_msg.type = BOARD;
        strncpy(new_msg.data.board, clients[msg->id].board, BOARD_SIZE);
        write(clients[clients[msg->id].opponent].sockfd, &new_msg, sizeof(struct message));
    }
    pthread_mutex_unlock(&clients[clients[msg->id].opponent].mutex);
}

void *start_routine_manage_sockets(void *arg) {
    struct epoll_event events[MAX_EVENTS];
    int nfds;
    struct message msg;

    while (1) {
        if ((nfds = epoll_wait(clients_epollfd, events, MAX_EVENTS, -1)) == -1) {
            ERROR(1,1, "Error: epoll wait failed\n");
        }
        for (int i=0; i<nfds; ++i) {
            read(events[i].data.fd, &msg, sizeof(struct message));
            printf("Recieved message from %d\n", events[i].data.fd);

            pthread_mutex_lock(&clients[msg.id].mutex);
            switch (msg.type) {
                case DISCONNECT:
                    handle_disconnect(msg.id);
                    break;
                case MOVE:
                    handle_move(&msg);
                    break;
                case PING:
                    printf("Read ping\n");
                    break;
                default:
                    ERROR(1, 0, "Error: received message with invalid type\n"); 
            }
            pthread_mutex_unlock(&clients[msg.id].mutex);
        }
    }
}

void creat_threads() {
    pthread_t ping_thread;
    if (pthread_create(&ping_thread, NULL, start_routine_ping, NULL) != 0) {
        ERROR(1, 1, "Error: unable to create pinging thread\n");
    }
    printf("Created ping thread\n");

    pthread_t manage_sockets_thread;
    if (pthread_create(&manage_sockets_thread, NULL, start_routine_manage_sockets, NULL) != 0) {
        ERROR(1, 1, "Error: unable to create pinging thread\n");
    }
    printf("Created main routine thread\n");

    struct epoll_event events[MAX_EVENTS];
    int nfds;
    struct message msg;

    while(1) {
        if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1) {
            ERROR(1,1, "Error: epoll wait failed\n");
        }
        for (int i=0; i<nfds; ++i) {
            int client_sockfd = accept(events->data.fd, NULL, NULL);
            printf("Accepted incoming connection, socked descriptor: %d\n", client_sockfd);
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
            printf("Registered client %s\n", clients[new_id].name);
            find_opponent(new_id);

            pthread_mutex_unlock(&clients_mutex);   
        }
    }
}

void handle_exit() {
    printf("Exiting...\n");
    struct message msg;
    msg.type = DISCONNECT;
    for (int i=0; i<MAX_PLAYERS; ++i) {
        if (clients[i].taken) {
            write(clients[i].sockfd, &msg, sizeof(struct message));
            close(clients[i].sockfd);
            if (clients[i].board != NULL) {
                free(clients[i].board);
                clients[clients[i].opponent].board = NULL;
            }
            pthread_mutex_destroy(&clients[i].mutex);
        }
    }
    shutdown(local_sockfd, SHUT_RDWR);
    shutdown(net_sockfd, SHUT_RDWR);
    close(local_sockfd);
    close(net_sockfd);
    unlink(path);
    close(epollfd);
    close(clients_epollfd);
}

void handle_signal(int signo) {
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        ERROR(1, 0, "Error: invalid number of arguments, expected 2\n");
    }

    atexit(handle_exit);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    srand(time(NULL));

    u_int16_t port = strtol(argv[1], NULL, 10);
    if (port != 0 && !(port > 1024 && port < 65535)) {
        ERROR(1, 0, "Error: invalid port, expected either 0 or one from range 1024-65535\n");
    }

    path = argv[2];
    if (strlen(path)+1 > UNIX_PATH_MAX) {
        ERROR(1, 0, "Error: unix socket path is too long\n");
    }
    
    creat_clients();
    creat_sockets(port);
    creat_epoll();
    creat_threads();

    return 1;
}
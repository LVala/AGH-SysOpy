#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <mqueue.h>
#include "consts.h"

#define QARRAY_SIZE 32

mqd_t sq_desc;
mqd_t clients[QARRAY_SIZE];

void create_server_queue(void) {
    struct mq_attr qattr;
    qattr.mq_maxmsg = 10;
    qattr.mq_msgsize = MSG_SIZE;

    mq_unlink(SERVER_PATH);
    sq_desc = mq_open(SERVER_PATH, O_RDONLY | O_CREAT, 0666, &qattr);
    if (sq_desc == -1) {
        ERROR(1, 1, "Error: server queue cannot be open\n");
    }
}

int find_free() {
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] == -1)
            return i;
    }
    return -1;
}

void handle_init(char *content) {
    int index = find_free();
    if (index == -1) {
        ERROR(0, 0, "Error: clients limit exceeded\n");
        return;
    }
    
    clients[index] = mq_open(content, O_WRONLY);
    if (clients[index] == -1) {
        ERROR(1, 1, "Error: client %d queue could not be opened\n", index);
    }

    char message[MSG_SIZE];
    message[0] = INIT;
    message[1] = index;
    message[2] = '\0';
    if (mq_send(clients[index], message, MSG_SIZE, INIT) == -1) {
        ERROR(1, 1, "Error: initial message do client %d could not be sent\n", index);
    }
}

void handle_stop(int sender_id) {
    if (clients[sender_id] != -1) {
        if (mq_close(clients[sender_id]) == -1) {
            ERROR(1, 1, "Error: client queue could not be closed\n");
        }
        clients[sender_id] = -1;
    }
    else {
        ERROR(1, 1, "Error: received stop message form client that does not exist\n");
    }
}

void handle_list(int sender_id) {
    char message[MSG_SIZE];
    message[0] = LIST;
    message[1] = '\0';

    char buffer[20];
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != -1) {
            sprintf(buffer, "%d ", i);
            if (strlen(message) + strlen(buffer) + 1 > MSG_SIZE) {
                ERROR(0, 0, "Error: list of clients is too long to be send\n");
                return;
            }
            strcat(message, buffer);
        }
    }

    if (mq_send(clients[sender_id], message, MSG_SIZE, LIST) == -1) {
        ERROR(1, 1, "Error: list message could not be send to client %d\n", sender_id);
    }
}

void handle_toall(int sender_id, char *content) {
    char message[MSG_SIZE];
    message[0] = TOALL;
    message[1] = sender_id;
    message[2] = '\0';

    time_t now_time = time(NULL);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S ", localtime(&now_time));
    strcat(message+2, buffer);
    strcat(message+2, content);

    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != -1) {
            if (mq_send(clients[i], message, MSG_SIZE, TOALL) == -1) {
                ERROR(1, 1, "Error: 2all message could not be send to client %d\n", i);
            }
        }
    }
}

void handle_toone(int sender_id, int receiver_id, char *content) {
    char message[MSG_SIZE];
    message[0] = TOONE;
    message[1] = sender_id;
    message[2] = '\0';

    time_t now_time = time(NULL);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S ", localtime(&now_time));
    strcat(message+2, buffer);
    strcat(message+2, content);


    if (clients[receiver_id] == -1) {
        ERROR(0, 0, "Error: 2one message receiver does not exist\n");
        return;
    }

    if (mq_send(clients[receiver_id], message, MSG_SIZE, TOONE) == -1) {
        ERROR(1, 1, "Error: 2one to client %d message could not be send\n", receiver_id);
    }
}

void close_on_exit(void) {
    char message[MSG_SIZE];
    message[0] = STOP;
    message[2] = '\0';

    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != -1) {
            if (mq_send(clients[i], message, MSG_SIZE, STOP) == -1) {
                ERROR(1, 1, "Error: stop message could not be send to client %d\n", i);
            }
        }
    }
    
    if (mq_close(sq_desc) == -1) {
        ERROR(1, 1, "Error: server queue could not be closed\n");
    }
    if (mq_unlink(SERVER_PATH) == -1) {
        ERROR(1, 1, "Error: server queue could not be unlinked\n");
    }
}

void sigint_handler(int exit_val) {
    exit(exit_val);
}

int main(void) {
    if (atexit(close_on_exit) == -1) {
        ERROR(1, 1, "Error: exit handler could not be set\n");
    }

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n")
    }

    FILE *log_file = fopen("./server_log.txt", "a");
    char time_buff[30];

    create_server_queue();

    for (int i=0; i<QARRAY_SIZE; ++i) clients[i] = -1;

    char message[MSG_SIZE];
    while(1) {
        if (mq_receive(sq_desc, message, MSG_SIZE, NULL) == -1) {
            ERROR(1, 1, "Error: message from client could not be received\n");
        }
        time_t now_time = time(NULL);
        strftime(time_buff, 30, "%Y-%m-%d %H:%M:%S ", localtime(&now_time));
        fprintf(log_file, "TIME: %s | SENDER_ID: %d | TYPE: %d | CONTENT: %s\n", \
            time_buff, message[1], message[0], message+2); 
        fflush(log_file);

        switch(message[0]) {
            case STOP:
                handle_stop(message[1]);
                break;
            case LIST:
                handle_list(message[1]);
                break;
            case TOALL:
                handle_toall(message[1], message+3);
                break;
            case TOONE:
                handle_toone(message[1], message[2], message+3);
                break;
            case INIT:
                handle_init(message+1);
                break;
            default:
                ERROR(0, 0, "Error: received message type does not exist\n");
                break;
        }
    }
    fclose(log_file);
    return 0;
}
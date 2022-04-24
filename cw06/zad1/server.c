#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "consts.h"

#define QARRAY_SIZE 32

int sq_desc;
int clients[QARRAY_SIZE];

void create_server_queue(void) {
    key_t key = ftok(SERVER_PATH, PROJ_ID);
    if (key == -1) {
        ERROR(1, 1, "Error: key cannot be generated\n");
    }

    sq_desc = msgget(key, IPC_CREAT | 0666);
    if (sq_desc == -1) {
        ERROR(1, 1, "Error: server queue cannot be open\n");
    }
}

int find_free() {
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] == 0)
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
    key_t key = strtol(content, NULL, 10);
    clients[index] = msgget(key, 0);
    if (clients[index] == -1) {
        ERROR(1, 1, "Error: client %d queue could not be opened\n", index);
    }

    struct Message feedback;
    feedback.type = INIT;
    feedback.receiver_id = index;
    if (msgsnd(clients[index], &feedback, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: initial message do client %d could not be sent\n", index);
    }
}

void handle_stop(int sender_id) {
    if (clients[sender_id] != 0) {
        clients[sender_id] = 0;
    }
    else {
        ERROR(1, 1, "Error: received stop message form client that does not exist\n");
    }
}

void handle_list(int sender_id) {
    struct Message feedback;
    feedback.type = LIST;
    feedback.content[0] = '\0';
    char buffer[20];
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != 0) {
            sprintf(buffer, "%d ", i);
            if (strlen(feedback.content) + strlen(buffer) + 1 > MSG_SIZE) {
                ERROR(0, 0, "Error: list of clients is too long to be send\n");
                return;
            }
            strcat(feedback.content, buffer);
        }
    }

    if (msgsnd(clients[sender_id], &feedback, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: list message could not be send to client %d\n", sender_id);
    }
}

void handle_toall(int sender_id, char *content) {
    struct Message feedback;
    feedback.type = TOALL;
    feedback.sender_id = sender_id;
    strcpy(feedback.content, content);
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != 0) {
            feedback.send_time = time(NULL);
            if (msgsnd(clients[i], &feedback, SEND_SIZE, 0) == -1) {
                ERROR(1, 1, "Error: 2all message could not be send to client %d\n", i);
            }
        }
    }
}

void handle_toone(int sender_id, int receiver_id, char *content) {
    struct Message feedback;
    feedback.type = TOALL;
    feedback.sender_id = sender_id;
    feedback.receiver_id = receiver_id;
    strcpy(feedback.content, content);

    if (clients[feedback.receiver_id] == 0) {
        ERROR(0, 0, "Error: 2one message receiver does not exist\n");
        return;
    }
    feedback.send_time = time(NULL);
    if (msgsnd(clients[feedback.receiver_id], &feedback, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: 2one to client %s message could not be send\n", feedback.receiver_id);
    }
}

void close_on_exit(void) {
    struct Message feedback;
    feedback.type = STOP;
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != 0) {
            if (msgsnd(clients[i], &feedback, SEND_SIZE, 0) == -1) {
                ERROR(1, 1, "Error: stop message could not be send to client %d\n", i);
            }
        }
    }
    
    if (msgctl(sq_desc, IPC_RMID, NULL) == -1) {
        ERROR(1, 1, "Error: server queue could not be removed\n");
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
    char time_buff[20];

    create_server_queue();

    for (int i=0; i<QARRAY_SIZE; ++i) clients[i] = 0;

    struct Message message;
    while(1) {
        if (msgrcv(sq_desc, &message, SEND_SIZE, -6, 0) == -1) {
            ERROR(1, 1, "Error: message form server queue could not be receieved\n");
        }
        if (&message != NULL) {
            time_t now_time = time(NULL);
            strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now_time));
            fprintf(log_file, "TIME: %s | SENDER_ID: %d | TYPE: %d | CONTENT: %s\n", \
                time_buff, message.sender_id, message.type, message.content);
            fflush(log_file);

            switch(message.type) {
                case STOP:
                    handle_stop(message.sender_id);
                    break;
                case LIST:
                    handle_list(message.sender_id);
                    break;
                case TOALL:
                    handle_toall(message.sender_id, message.content);
                    break;
                case TOONE:
                    handle_toone(message.sender_id, message.receiver_id, message.content);
                    break;
                case INIT:
                    handle_init(message.content);
                    break;
                default:
                    ERROR(0, 0, "Error: received message type does not exist\n");
                    break;
            }
        }
    }
    fclose(log_file);
    return 0;
}
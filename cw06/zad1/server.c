#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "consts.h"

#define QARRAY_SIZE 32

int create_server_queue(key_t *key) {
    // char *home_path = getenv("HOME");
    // if (home_path == NULL) {
    //     //ERROR
    // }

    if ((*key = ftok(SERVER_PATH, PROJ_ID)) == -1) {
        //ERROR
    }

    int sq_desc = msgget(*key, IPC_CREAT | IPC_EXCL);
    if (sq_desc == -1) {
        //ERROR
    }

    return sq_desc;
}

int find_free(int *clients) {
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] == 0)
            return i;
    }
    return -1;
}

void handle_init(struct Message *message, int *clients) {
    int index = find_free(clients);
    if (index == -1) {
        //ERROR
    }
    key_t key = strtol(message->content, NULL, 10);
    
    clients[index] = msgget(key, 0);
    if (clients[index] == -1) {
        //ERROR
    }

    struct Message feedback;
    feedback.type = INIT;
    feedback.receiver_id = index;

    if (msgsnd(clients[index], &feedback, sizeof(struct Message), 0) == -1) {
        //ERROR
    }
}

void handle_stop(struct Message *message, int *clients) {
    if (clients[message->sender_id] != 0) {
        clients[message->sender_id] = 0;
    }
    else {
        //ERROR
    }
}

void handle_list(struct Message *message, int *clients) {
    struct Message feedback;
    feedback.type = LIST;
    feedback.receiver_id = message->sender_id;
    feedback.content[0] = '\0';
    char buffer[20];
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != 0) {
            sprintf(buffer, "%d", i);
            if (strlen(feedback.content) + strlen(buffer) + 1 < MSG_SIZE) {
                strcat(feedback.content, buffer);
            }
            else {
                //ERROR
            }
        }
    }

    if (msgsnd(clients[message->sender_id], &feedback, sizeof(struct Message), 0) == -1) {
        //ERROR
    }
}

void handle_toall(struct Message *message, int *clients) {
    struct Message feedback;
    feedback.type = TOALL;
    feedback.sender_id = message->sender_id;
    strcpy(feedback.content, message->content);
    for (int i=0; i<QARRAY_SIZE; ++i) {
        if (clients[i] != 0) {
            feedback.receiver_id = i;
            feedback.send_time = time(NULL);
            if (msgsnd(clients[i], &feedback, sizeof(struct Message), 0) == -1) {
                //ERROR
            }
        }
    }
}

void handle_toone(struct Message *message, int *clients) {
    struct Message feedback;
    feedback.type = TOALL;
    feedback.sender_id = message->sender_id;
    feedback.receiver_id = message->receiver_id;
    strcpy(feedback.content, message->content);

    if (clients[feedback.receiver_id] != 0) {
        feedback.send_time = time(NULL);
        if (msgsnd(clients[feedback.receiver_id], &feedback, sizeof(struct Message), 0) == -1) {
            //ERROR
        }
    }
}

int main(void) {
    key_t sq_key;
    int sq_desc = create_server_queue(&sq_key);

    int clients[QARRAY_SIZE];
    for (int i=0; i<QARRAY_SIZE; ++i) clients[i] = 0;

    struct Message message;
    while(1) {
        if (msgrcv(sq_desc, &message, sizeof(struct Message), -6, 0) == -1) {
            //ERROR
        }

        if (&message != NULL) {
            switch(message.type) {
                case STOP:
                    handle_stop(&message, clients);
                    break;
                case LIST:
                    handle_list(&message, clients);
                    break;
                case TOALL:
                    handle_toall(&message, clients);
                    break;
                case TOONE:
                    handle_toone(&message, clients);
                    break;
                case INIT:
                    handle_init(&message, clients);
                    break;
                default:
                    //ERROR
                    break;
            }
        }
    }

    return 0;
}
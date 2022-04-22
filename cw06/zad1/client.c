#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "consts.h"

int initialize(int *sq_desc, int *cq_desc) {
    // server
    key_t skey = ftok(SERVER_PATH, PROJ_ID);
    if (skey == -1) {
        //ERROR
    }
    *sq_desc = msgget(skey, 0);
    if (*sq_desc == -1) {
        //ERROR
    }

    // client
    char *home_path = getenv("HOME");
    if (home_path == NULL) {
        //ERROR
    }
    key_t ckey = ftok(home_path, PROJ_ID);
    if (ckey == -1) {
        //ERROR
    }
    *cq_desc = msgget(ckey, 0);
    if (*cq_desc == -1) {
        //ERROR
    }

    struct Message message;
    message.type = INIT;
    sprintf(message.content, "%d", ckey);
    if (msgsnd(*sq_desc, &message, sizeof(struct Message), 0) == -1) {
        //ERROR
    }

    struct Message feedback;
    if (msgrcv(cq_desc, &feedback, sizeof(struct Message), INIT,  0) == -1) {
        //ERROR
    }

    return feedback.receiver_id;
}

void handle_list(int client_id, int *sq_desc, int *cq_desc) {
    struct Message message;
    message.sender_id = client_id;
    message.type = LIST;
    if (msgsnd(*sq_desc, &message, sizeof(struct Message), 0) == -1) {
        //ERROR
    }

    struct Message feedback;
    if (msgrcv(cq_desc, &feedback, sizeof(struct Message), INIT,  0) == -1) {
        //ERROR
    }

    printf("CLIENT %d\n    LIST OF ACTIVE CLIENTS: %s\n", client_id, feedback.content);
}

void handle_toall(int client_id, int *sq_desc, int *cq_desc, char *str) {
    struct Message message;
    message.sender_id = client_id;
    message.type = TOALL;
    strcpy(message.content, str);
    if (msgsnd(*sq_desc, &message, sizeof(struct Message), 0) == -1) {
        //ERROR
    }

    //TODO FEEDBACK
}

void handle_toone(int client_id, int *sq_desc, int *cq_desc, char *str) {

}

void handle_stop(int client_id, int *sq_desc, int *cq_desc) {

}

int main() {
    int sq_desc;
    int cq_desc;
    int client_id = initialize(&sq_desc, &cq_desc);


    return 0;
}
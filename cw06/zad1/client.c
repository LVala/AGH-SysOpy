#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "consts.h"

int sq_desc;
int cq_desc;
int client_id;

void initialize(void) {
    // server
    key_t skey = ftok(SERVER_PATH, PROJ_ID);
    if (skey == -1) {
        ERROR(1, 1, "Error: server key could not be generated\n");
    }
    sq_desc = msgget(skey, 0);
    if (sq_desc == -1) {
        ERROR(1, 1, "Error: server queue could not be opened\n");
    }

    // client
    char *home_path = getenv("HOME");
    if (home_path == NULL) {
        ERROR(1, 1, "Error: home path could not be obtained\n");
    }
    key_t ckey = ftok(home_path, PROJ_ID);
    if (ckey == -1) {
        ERROR(1, 1, "Error: client key could not be generated\n");
    }
    cq_desc = msgget(ckey, IPC_CREAT | IPC_EXCL | 0666);
    if (cq_desc == -1) {
        ERROR(1, 1, "Error: client queue could not be created\n");
    }

    struct Message message;
    message.type = INIT;
    sprintf(message.content, "%d", ckey);
    if (msgsnd(sq_desc, &message, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: initial message to server could not be sent\n");
    }

    if (msgrcv(cq_desc, &message, SEND_SIZE, INIT,  0) == -1) {
        ERROR(1, 1, "Error: initial message from server could not be received\n");
    }
    
    client_id = message.receiver_id;
}

void send_list() {
    struct Message message;
    message.sender_id = client_id;
    message.type = LIST;
    if (msgsnd(sq_desc, &message, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: list message could not be sent\n")
    }

    struct Message feedback;
    if (msgrcv(cq_desc, &feedback, SEND_SIZE, LIST,  0) == -1) {
        ERROR(1, 1, "Error: list message could not be received\n");
    }

    printf("CLIENT %d | LIST OF ACTIVE CLIENTS: %s\n", client_id, feedback.content);
}

void send_toall(char *str) {
    struct Message message;
    message.sender_id = client_id;
    message.type = TOALL;
    if (strlen(str) > MSG_SIZE + 1) {
        ERROR(0, 0, "Error: message exceeds maximum size\n");
        return;
    }
    strcpy(message.content, str);
    if (msgsnd(sq_desc, &message, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: 2all message could not be sent\n");
    }
}

void send_toone(int dest_id, char *str) {
    struct Message message;
    message.sender_id = client_id;
    message.receiver_id = dest_id;
    message.type = TOONE;
    if (strlen(str) > MSG_SIZE + 1) {
        ERROR(0, 0, "Error: message exceeds maximum size\n");
        return;
    }
    strcpy(message.content, str);
    if (msgsnd(sq_desc, &message, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: 2one message could not be sent\n");
    }
}

void send_stop(int exit_val) {
    struct Message message;
    message.sender_id = client_id;
    message.type = STOP;
    if (msgsnd(sq_desc, &message, SEND_SIZE, 0) == -1) {
        ERROR(1, 1, "Error: stop message could not be sent\n");
    }
    exit(exit_val);
}

void close_on_exit(void) {
    if (msgctl(cq_desc, IPC_RMID, NULL) == -1) {
        ERROR(1, 1, "Error: client queue could not be removed\n");
    }
}

int check_for_input(void) {
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    return (FD_ISSET(0, &rfds));
}

void handle_message_to(int sender_id, time_t send_time, char *content) {
    char buff[20];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&send_time));
    printf("CLIENT %d | MESSAGE FROM %d at %s:\n", client_id, sender_id, buff);
    printf("    %s\n", content);
}

int main() {
    if (atexit(close_on_exit) == -1) {
        ERROR(1, 1, "Error: exit handler could not be set\n");
    }

    if (signal(SIGINT, send_stop) == SIG_ERR) {
        ERROR(1, 1, "Error: SIGINT handler could not be set\n")
    }

    initialize();

    struct msqid_ds queue;
    struct Message message;
    char input_buffer[MSG_SIZE+20];
    
    printf("CLIENT %d | Enter commands:\n", client_id);
    fflush(stdout);
    while(1) {
        if (msgctl(cq_desc, IPC_STAT, &queue)) {
            ERROR(1, 1, "Error: client queue status could not be obtained\n");
        }
        if (queue.msg_qnum > 0) {
            if (msgrcv(cq_desc, &message, SEND_SIZE, 0, 0) == -1) {
                ERROR(1, 1, "Error: message from client queue could not be obtained\n");
            }
            switch(message.type) {
                case STOP:
                    send_stop(0);
                    break;
                case TOALL: case TOONE:
                    handle_message_to(message.sender_id, message.send_time, message.content);
                    break;
                default:
                    ERROR(0, 0, "Error: received message type does not exist\n")
            }
        }
        else if (check_for_input()) {
            if (fgets(input_buffer, MSG_SIZE+20, stdin) == NULL) {
                ERROR(1, 1, "Error: user input could not be obtained\n");
            }
            if (input_buffer[strlen(input_buffer)-1] == '\n')
                input_buffer[strlen(input_buffer)-1] = '\0';
            
            char *ptr;
            char *token = strtok_r(input_buffer, " \0", &ptr);
            if (!strcmp(token, "LIST")) {
                send_list();
            }
            else if (!strcmp(token, "STOP")) {
                send_stop(0);
            }
            else if (!strcmp(token, "2ALL")) {
                send_toall(ptr);
            }
            else if (!strcmp(token, "2ONE")) {
                char *token2 = strtok_r(NULL, " \0", &ptr);
                errno = 0;
                int desc_id = strtol(token2, NULL, 10);
                if (desc_id == 0 && errno != 0) {
                    ERROR(0, 0, "Error: invalid value representing reveiver id\n");
                    break;
                }
                send_toone(desc_id, ptr);
            }
            else {
                ERROR(0, 0, "Error: invalid message type\n");
            }
        }
        else {
            usleep(5000);     
        }
    }
    return 0;
}
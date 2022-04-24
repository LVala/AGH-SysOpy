#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <mqueue.h>
#include "consts.h"

int sq_desc;
int cq_desc;
int client_id;

void initialize(void) {
    // server
    sq_desc = mq_open(SERVER_PATH, O_WRONLY);
    if (sq_desc == -1) {
        ERROR(1, 1, "Error: server queue cannot be open\n");
    }

    // client
    struct mq_attr qattr;
    qattr.mq_maxmsg = 10;
    qattr.mq_msgsize = MSG_SIZE;

    char *home_path = getenv("HOME");
    if (home_path == NULL) {
        ERROR(1, 1, "Error: home path could not be obtained\n");
    }
    char home_buffer[20];
    strcpy(home_buffer, home_path);

    char *ptr;
    strtok_r(home_buffer, "/", &ptr);
    char q_name[30];
    q_name[0] = '/';
    q_name[1] = '\0';
    strcat(q_name, ptr);

    mq_unlink(q_name);
    cq_desc = mq_open(q_name, O_RDONLY | O_CREAT, 0666, &qattr);
    if (cq_desc == -1) {
        ERROR(1, 1, "Error: client queue could not be opened\n");
    }

    char message[MSG_SIZE];
    message[0] = INIT;
    message[1] = '\0';
    strcat(message, q_name);

    if (mq_send(sq_desc, message, MSG_SIZE, INIT) == -1) {
        ERROR(1, 1, "Error: initial message to server could not be sent\n");
    }

    if (mq_receive(cq_desc, message, MSG_SIZE, NULL) == -1) {
        ERROR(1, 1, "Error: initial message from server could not be received\n");
    }
    
    client_id = message[1];
}

void send_list() {
    char message[MSG_SIZE];
    message[0] = LIST;
    message[1] = client_id;
    message[2] = '\0';
    if (mq_send(sq_desc, message, MSG_SIZE, LIST) == -1) {
        ERROR(1, 1, "Error: initial message to server could not be sent\n");
    }

    if (mq_receive(cq_desc, message, MSG_SIZE, NULL) == -1) {
        ERROR(1, 1, "Error: initial message from server could not be received\n");
    }

    printf("CLIENT %d | LIST OF ACTIVE CLIENTS: %s\n", client_id, message+1);
}

void send_toall(char *str) {
    char message[MSG_SIZE];
    message[0] = TOALL;
    message[1] = client_id;
    message[2] = 1;
    message[3] = '\0';

    if (strlen(str) > MSG_SIZE + 1) {
        ERROR(0, 0, "Error: message exceeds maximum size\n");
        return;
    }
    strcat(message+3, str);

    if (mq_send(sq_desc, message, MSG_SIZE, TOALL) == -1) {
        ERROR(1, 1, "Error: 2all message could not be sent\n");
    }
}

void send_toone(int dest_id, char *str) {
    char message[MSG_SIZE];
    message[0] = TOONE;
    message[1] = client_id;
    message[2] = dest_id;
    message[3] = '\0';
    if (strlen(str) > MSG_SIZE + 1) {
        ERROR(0, 0, "Error: message exceeds maximum size\n");
        return;
    }
    strcpy(message+3, str);
    if (mq_send(sq_desc, message, MSG_SIZE, TOONE) == -1) {
        ERROR(1, 1, "Error: 2all message could not be sent\n");
    }
}

void send_stop(int exit_val) {
    char message[MSG_SIZE];
    message[0] = STOP;
    message[1] = client_id;
    message[2] = '\0';
    if (mq_send(sq_desc, message, MSG_SIZE, STOP) == -1) {
        ERROR(1, 1, "Error: 2all message could not be sent\n");
    }
    exit(exit_val);
}

void close_on_exit(void) {
    if (mq_close(sq_desc) == -1) {
        ERROR(1, 1, "Error: server queue could not be closed\n");
    }

    if (mq_close(cq_desc) == -1) {
        ERROR(1, 1, "Error: client queue could not be closed\n");
    }

    char *home_path = getenv("HOME");
    if (home_path == NULL) {
        ERROR(1, 1, "Error: home path could not be obtained\n");
    }
    char home_buffer[20];
    strcpy(home_buffer, home_path);

    char *ptr;
    strtok_r(home_buffer, "/", &ptr);
    char q_name[30];
    q_name[0] = '/';
    q_name[1] = '\0';
    strcat(q_name, ptr);

    if (mq_unlink(q_name) == -1) {
        ERROR(1, 1, "Error: client queue could not be unlinked\n");
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

void handle_message_to(int sender_id, char *send_time, char *content) {
    printf("CLIENT %d | MESSAGE FROM %d at %s:\n", client_id, sender_id, send_time);
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

    char input_buffer[MSG_SIZE];
    char message[MSG_SIZE];
    struct mq_attr qattr;
    
    printf("CLIENT %d | Enter commands:\n", client_id);
    fflush(stdout);
    while(1) {
        if (mq_getattr(cq_desc, &qattr) == -1) {
            ERROR(1, 1, "Error: client queue status could not be obtained\n");
        }
        if (qattr.mq_curmsgs > 0) {
            if (mq_receive(cq_desc, message, MSG_SIZE, NULL) == -1) {
                ERROR(1, 1, "Error: message from client queue could not be obtained\n");
            }
            switch(message[0]) {
                case STOP:
                    send_stop(0);
                    break;
                case TOALL: case TOONE:
                    char *ptr;
                    char *token = strtok_r(message+2, " ", &ptr);
                    strtok_r(NULL, " ", &ptr);
                    token[strlen(token)] = ' ';
                    handle_message_to(message[1], token, ptr);
                    break;
                default:
                    ERROR(0, 0, "Error: received message type does not exist\n")
            }
        }
        else if (check_for_input()) {
            if (fgets(input_buffer, MSG_SIZE, stdin) == NULL) {
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
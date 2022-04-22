#ifndef CONST_H
#define CONST_H

#define MSG_SIZE 1024  // maximum message size
#define SERVER_PATH "/"
#define PROJ_ID 1337   // project id used in key generating

#define STOP  1
#define LIST  2
#define TOALL 3
#define TOONE 4
#define INIT  5

struct Message {
    long type;
    int sender_id;
    int receiver_id;
    time_t send_time;
    char content[MSG_SIZE];
};

#endif
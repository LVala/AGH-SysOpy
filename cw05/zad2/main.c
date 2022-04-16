#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void list_mails(char *sort_key) {
    FILE *stream;
    if (!strcmp(sort_key, "data")) {
        stream = popen("echo h | mail | sort -k4,6 ", "r");  // this is very wrong btw
    }
    else if (!strcmp(sort_key, "nadawca")) {
        stream = popen("echo h | mail | sort -k3", "r");
    }
    else {
        fprintf(stderr, "Invalid argument, choose one from {data, nadawca}\n");
        exit(1);
    }
    if (stream == NULL) {
        fprintf(stderr, "Popen failed: %s\n", strerror(errno));
        exit(1);
    }

    pclose(stream);
}

void send_mail(char *reciever, char *title, char *content) {
    char *command = calloc(strlen(reciever) + strlen(title) + 30, sizeof(char));
    if (command == 0) {
        fprintf(stderr, "Memory allocation failed: %s", strerror(errno));
        exit(1);
    }
    sprintf(command, "mail -s %s %s", title, reciever);
    FILE *stream = popen(command, "w");
    if (stream == NULL) {
        fprintf(stderr, "Popen failed: %s\n", strerror(errno));
        exit(1);
    }
    fputs(command, stream);

    pclose(stream);
    free(command);
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(1);
    }

    if (argc == 2)
        list_mails(argv[1]);
    
    if (argc == 4)
        send_mail(argv[1], argv[2], argv[3]);

    return 0;
}
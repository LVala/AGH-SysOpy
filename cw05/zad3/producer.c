#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

int isnumber(const char *ptr) {
    if (ptr == NULL)
        return 1;
    while (*ptr != '\0') {
        if (!isdigit(*ptr))
            return 0;
        ptr++;
    }
    return 1;
}

void remove_newlines(char *array) {
    while (*array++)
        if (*array == '\n')
            *array = '\t';
}

void read_to_fifo(char *fifo_path, char *row_num, char *file_path, int chars_num) {
    FILE *fifop = fopen(fifo_path, "a");
    if (fifop == NULL) {
        fprintf(stderr, "Fifo could not be opened: %s\n", strerror(errno));
        exit(1);
    }
    FILE *filep = fopen(file_path, "r");
    if (filep == NULL) {
        fprintf(stderr, "File could not be opened: %s\n", strerror(errno));
        exit(1);
    }

    char *buffer = calloc(chars_num + strlen(row_num) + 1, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    strcpy(buffer, row_num);
    buffer[strlen(row_num)] = ' ';

    int len;
    while ((len = fread(buffer+strlen(row_num)+1, sizeof(char), chars_num, filep))) {
        if (len < chars_num)
            buffer[len + strlen(row_num)+1] = '\0';

        struct timespec tim;
        tim.tv_sec = 1;
        tim.tv_nsec = (rand()%1000)*1000000;

        if(nanosleep(&tim, NULL) < 0 ) {
            printf("Nanosleep failed: %s\n", strerror(errno));
            exit(1);
        }
        // gets rid of newline characters in order to store it in one line
        remove_newlines(buffer);
        if (fwrite(buffer, sizeof(char), chars_num + strlen(row_num) + 1, fifop) <= 0) {
            fprintf(stderr, "Writing to fifo failed: %s\n", strerror(errno));
            exit(1);
        }
        fflush(fifop);
    }

    free(buffer);
    fclose(filep);
    fclose(fifop);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    if (!isnumber(argv[2])) {
        fprintf(stderr, "Line number value is invalid, must be positive\n");
        exit(1);
    }
    int chars_num = strtol(argv[4], NULL, 10);
    if (chars_num <= 0) {
        fprintf(stderr, "Number of chars to copy is invalid, must be positive\n");
        exit(1);
    }

    read_to_fifo(argv[1], argv[2], argv[3], chars_num);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void add_line(FILE *filep, FILE *newfilep, char *to_add, int row_num, int chars_num) {
    char *buffer = calloc(chars_num, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }
    
    int i=0;
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, filep) != -1) {
        ++i;
        if (i == row_num) {
            if (line[strlen(line)-1] == '\n')
                line[strlen(line)-1] = '\0';
            if (fputs(line, newfilep) == EOF || fputs(to_add, newfilep) == EOF || fputc('\n', newfilep) == EOF) {
                fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
                exit(1);
            }
        }
        else {
            if(fputs(line, newfilep) == EOF) {
                fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
                exit(1);
            }
        }
    }

    if (i < row_num) {
        while (++i != row_num)
            if (fputc('\n', newfilep) == EOF) {
                fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
                exit(1);
            }
        
        if (fputs(to_add, newfilep) == EOF || fputc('\n', newfilep) == EOF) {
            fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
            exit(1);
        }
    }


    fflush(newfilep);
    fseek(newfilep, 0, SEEK_SET);
    free(line);
    free(buffer);
}

void read_from_fifo(char *fifo_path, char *file_path, int chars_num) {
    FILE *fifop = fopen(fifo_path, "r");
    if (fifop == NULL) {
        fprintf(stderr, "Fifo could not be opened: %s\n", strerror(errno));
        exit(1);
    }
    FILE *filep = fopen(file_path, "r");
    if (filep == NULL) {
        fprintf(stderr, "File could not be opened: %s\n", strerror(errno));
        exit(1);
    }

    char *buffer = calloc(chars_num + 1, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    char row_num_arr[20], c;
    int len, row_num, i = 0;

    while ((c = fgetc(fifop)) != EOF) {
        if (c == ' ') {
            row_num_arr[i] = '\0';
            i = 0;
            row_num = strtol(row_num_arr, NULL, 10);
            if (row_num <= 0) {
                fprintf(stderr, "Reading row number from fifo failed\n");
                exit(1);
            }
            unlink(file_path);
            FILE *newfilep = fopen(file_path, "w+");

            if ((len = fread(buffer, sizeof(char), chars_num, fifop)) == 0) {
                fprintf(stderr, "Reading content from fifo failed\n");
                exit(1);
            }
            buffer[len] = '\0';
            add_line(filep, newfilep, buffer, row_num, chars_num);
            fclose(filep);
            filep = newfilep;
        }
        else {
            row_num_arr[i++] = c;
        }
    }

    free(buffer);
    fclose(filep);
    fclose(fifop);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    int chars_num = strtol(argv[3], NULL, 10);
    if (chars_num <= 0) {
        fprintf(stderr, "Number of chars to copy is invalid, must be positive\n");
        exit(1);
    }

    read_from_fifo(argv[1], argv[2], chars_num);

    return 0;
}
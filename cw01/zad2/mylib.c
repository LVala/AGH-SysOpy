#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mylib.h"

static char *create_command(size_t length, char *files[], char *temp_filename) {
    char *prg = "wc ";
    char *redir = "1>>";
    char *target;

    int char_quant = strlen(prg) + strlen(redir) + strlen(temp_filename) + 1;
    for (int i = 0; i < length; i++)
        char_quant += strlen(files[i]) + 1;  // +1 for space after, space for '/0' not needed

    char *command = calloc(char_quant, sizeof(char));
    if (command == NULL) {
        fprintf(stderr, "Dynamic memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    strcat(command, prg);
    target = command + strlen(prg);
    for (int i = 0; i < length; i++) {
        strcat(target, files[i]);
        target += strlen(files[i]);
        *target = ' ';  // this string is not terminated by '\0', but the pointer is moved, so it's ok
        target += 1;
    }
    strcat(target, redir);
    target += strlen(redir);
    strcat(target, temp_filename);

    return command;
}

static int find_free_index(struct Block *blocks, size_t length) {
    // not very elegant
    int i = 0;
    while (blocks[i].files != NULL && i < length)
        i++;
    if (i == length) {
        fprintf(stderr, "Warning: no more free indicies in the main array, remove blocks to add more\n");
        return -1;
    }
    return i;
}

struct Block *create_array(size_t array_size) {
    if (array_size <= 0) {
        fprintf(stderr, "Error: array size must positive\n");
        return NULL;
    }
    struct Block *blocks;
    blocks = calloc(array_size, sizeof(struct Block));
    if (blocks == NULL) {
        fprintf(stderr, "Dynamic memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }
    return blocks;
}

temp_file_handle count_to_file(size_t length, char *files[]) {
    if (length <= 0) {
        fprintf(stderr, "Error: number of arguments for wc must be positive\n");
        return -1;
    }
    char temp_filename[] = "/tmp/temp_file.XXXXXX";
    int fd = mkstemp(temp_filename);
    if (fd == -1) {
        fprintf(stderr, "Temporary file creation failed: %s\n", strerror(errno));
        exit(1);
    }
    char *command = create_command(length, files, temp_filename);

    FILE *file = fdopen(fd, "a");
    fprintf(file, "%ld\n", length);  // write number of lines to file
    fflush(file);
    if (system(command) == -1) {
        fprintf(stderr, "Error occured when running wc: %s\n", strerror(errno));
        exit(1);
    };
    
    unlink(temp_filename);
    free(command);
    return fd;
}

int file_to_memory(struct Block *blocks, size_t blocks_length, temp_file_handle fd) {
    if (blocks_length <= 0) {
        fprintf(stderr, "Warning: array size must positive or array does not exist\n");
        return -1;
    }
    size_t buffer_size = 1024;
    char *buffer = malloc(buffer_size * sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Dynamic memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    int index = find_free_index(blocks, blocks_length);
    if (index == -1)
        return -1;  // array is full

    FILE *file = fdopen(fd, "r");
    lseek (fd, 0, SEEK_SET); 
    char *ptr, *name;

    getline(&buffer, &buffer_size, file);
    blocks[index].length = strtol(buffer, &ptr, 10);
    blocks[index].files = calloc(blocks[index].length, sizeof(struct FileInfo));
    if (blocks[index].files == NULL) {
        fprintf(stderr, "Dynamic memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }
    int i;
    for (i = 0; i < blocks[index].length; i++) {
        if (getline(&buffer, &buffer_size, file) == -1)
            break;  // in a case all of wc input files are invalid
        blocks[index].files[i].lines = strtol(strtok(buffer, " \t\n\v\f\r"), &ptr, 10);
        blocks[index].files[i].words = strtol(strtok(NULL, " \t\n\v\f\r"), &ptr, 10);
        blocks[index].files[i].chars = strtol(strtok(NULL, " \t\n\v\f\r"), &ptr, 10);
        blocks[index].files[i].filename = calloc(strlen(name = strtok(NULL, " \t\n\v\f\r")) + 1, sizeof(char));
        if (blocks[index].files[i].filename == NULL) {
            fprintf(stderr, "Dynamic memory allocation failed: %s\n", strerror(errno));
            exit(1);
        }
        strcpy(blocks[index].files[i].filename, name);
    }
    if (getline(&buffer, &buffer_size, file) == -1 && !strcmp(name, "total") && blocks[index].length > 1) {
            // when some filenames are invalid, number of lines written to file is greater
            // than actual number of lines, which causes "total" line of wc output to be saved, which is unnecessary
            free(blocks[index].files[i-1].filename);
            blocks[index].length = i-1;
        };

    fclose(file);
    close(fd);
    free(buffer);

    return index;
}

int free_block(struct Block *blocks, size_t length, int index) {
    if (index < 0 || index >= length) {
        fprintf(stderr, "Warning: index to free out of array bounds\n");
        return -1;
    }
    if (blocks[index].files != NULL)
        for (int i = 0; i < blocks[index].length; i++)
            free(blocks[index].files[i].filename);

    free(blocks[index].files);
    blocks[index].files = NULL;
    return 0;
}

void free_block_array(struct Block *blocks, size_t length) {
    if (blocks != NULL)
        for (int i = 0; i < length; i++)
            free_block(blocks, length, i);
    free(blocks);
    blocks = NULL;
}

// int main(int argc, char *argv[]) {
//     struct Block *blocks = create_array(1);
//     temp_file_handle fd = count_to_file(argc-1, argv+1);
//     file_to_memory(blocks, argc - 1, fd);
//     free_block_array(blocks, 1);
//     return 0;
// }

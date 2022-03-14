#ifndef MYLIB_H
#define MYLIB_H

#include <stdlib.h>
#include <stdio.h>

typedef int temp_file_handle;

struct FileInfo {
    int lines;
    int words;
    int chars;
    char *filename;
};

struct Block {
    size_t length;
    struct FileInfo *files;
};

struct Block *create_array(size_t array_size);
temp_file_handle count_to_file(size_t length, char *files[]);
int file_to_memory(struct Block *blocks, size_t length, temp_file_handle fd);
int free_block(struct Block *blocks, size_t length, int index);
void free_block_array(struct Block *blocks, size_t length);

#endif  // MYLIB_H
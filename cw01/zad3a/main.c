#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/times.h>
#include <inttypes.h>

#include "mylib.h"

#ifndef RAPORT // raport name should be defined by a gcc flag
    #define RAPORT "raport.txt"
#endif

static int isnumber(const char *ptr) {
    if (ptr == NULL)
        return 1;
    while (*ptr != '\0') {
        if (!isdigit(*ptr))
            return 0;
        ptr++;
    }
    return 1;
}

static void output_raport(clock_t real_time_start, clock_t real_time_end, struct tms *time_start, struct tms *time_end) {

    FILE *ptr = fopen(RAPORT, "a");
    if (ptr == NULL) {
        fprintf(stderr, "Raport file could not be open or created: %s\n", strerror(errno));
        exit(1);
    }

    fprintf(ptr, "Operation timing raport:\n");
    fprintf(ptr, "    REAL: %f s\n", (double)(real_time_end - real_time_start) / (double)sysconf(_SC_CLK_TCK));
    fprintf(ptr, "    USER CPU: %f s\n", (double)(time_end->tms_cutime - time_start->tms_cutime) / (double)sysconf(_SC_CLK_TCK));
    fprintf(ptr, "    SYSTEM CPU: %f s\n\n", (double)(time_end->tms_cstime - time_start->tms_cstime) / (double)sysconf(_SC_CLK_TCK));

    printf("Operation timing raport:\n");
    printf("    REAL: %f s\n", (double)(real_time_end - real_time_start) / (double)sysconf(_SC_CLK_TCK));
    printf("    USER CPU: %f s\n", (double)(time_end->tms_cutime - time_start->tms_cutime) / (double)sysconf(_SC_CLK_TCK));
    printf("    SYSTEM CPU: %f s\n\n", (double)(time_end->tms_cstime - time_start->tms_cstime) / (double)sysconf(_SC_CLK_TCK));

    fclose(ptr);
}

int run_commands(int arg_length, char *args[]) {

#ifdef DYNAMIC
    void* handle = dlopen("libsharedmylib.so", RTLD_NOW);
    if (handle == 0) {
        fprintf(stderr, "Error: library could not be dynamically accessed\n");
        exit(1);
    }
    struct Block *(*create_array)(size_t array_size) = dlsym(handle, "create_array");
    temp_file_handle (*count_to_file)(size_t length, char *files[]) = dlsym(handle, "count_to_file");
    int (*file_to_memory)(struct Block *blocks, size_t length, temp_file_handle fd) = dlsym(handle, "file_to_memory");
    int (*free_block)(struct Block *blocks, size_t length, int index) = dlsym(handle, "free_block");
    void (*free_block_array)(struct Block *blocks, size_t length) = dlsym(handle, "free_block_array");   
#endif

    if (arg_length <= 0) {
        fprintf(stderr, "Error: No arguments were specified\n");
        exit(1);
    }

    struct Block *blocks = NULL;
    size_t blocks_length = 0;
    int i = 0;
    char *ptr;
    
    while (i < arg_length) {
        if (!strcmp(args[i], "create_table")) {
            if (i + 1 >= arg_length) {
                fprintf(stderr, "Error: create_table argument was not specified\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            if (!isnumber(args[i+1])) {
                fprintf(stderr, "Error: create_table argument must be a positive number\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            blocks_length = strtol(args[i+1], &ptr, 10);
            if (blocks_length == 0) {
                fprintf(stderr, "Error: create_table argument must be a positive number\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            free_block_array(blocks, blocks_length);
            blocks = create_array(blocks_length);
            i += 2;
        }
        else if (!strcmp(args[i], "remove_block")) {
            if (i + 1 >= arg_length) {
                fprintf(stderr, "Error: remove_block argument was not specified\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            if (!isnumber(args[i+1])) {
                fprintf(stderr, "Error: remove_block argument must be a non negative number\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            size_t block_to_remove = strtol(args[i+1], &ptr, 10);
            if (block_to_remove >= blocks_length) {
                fprintf(stderr, "Error: remove_block argument must be smaller than blocks_table length\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            if (blocks == NULL) {
                fprintf(stderr, "Error: blocks_table does not exist\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            free_block(blocks, blocks_length, block_to_remove);
            i += 2;
        }
        else if (!strcmp(args[i], "wc_files")) {
            if (blocks == NULL) {
                fprintf(stderr, "Error: blocks_table does not exist\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }

            int j = 1;
            while (i+j < arg_length && strcmp(args[i+j], "wc_files") && strcmp(args[i+j], "create_table") && strcmp(args[i+j], "remove_block")) {
                ++j;
            }
            if (j-1 == 0) {
                fprintf(stderr, "Error: wc_files arguments not specified\n");
                free_block_array(blocks, blocks_length);
                exit(1);
            }
            temp_file_handle fd = count_to_file(j-1, &args[i+1]);
            file_to_memory(blocks, blocks_length, fd);
            i += j;

        }
        else {
            fprintf(stderr, "Error: invalid command\n");
            free_block_array(blocks, blocks_length);
            exit(1);
        }
    }
    free_block_array(blocks, blocks_length);

#ifdef DYNAMIC
    dlclose(handle);
#endif

    return 0;
}

int main(int argc, char *argv[]) {
    clock_t real_time_start, real_time_end;
    struct tms time_start, time_end;

    real_time_start = times(&time_start);
    run_commands(argc - 1, argv + 1);
    real_time_end = times(&time_end);

    output_raport(real_time_start, real_time_end, &time_start, &time_end);
    
    return 0;
}
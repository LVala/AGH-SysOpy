#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include "timing.h"

int copy_and_remove_blanks(FILE *from_file, FILE *to_file) {
    size_t buffer_size = 1024;  // buffer size must be >= longest line length
    char buffer[buffer_size];

    size_t copied_size;
    int i, cur_start, is_blank, written_size;
    do {
        copied_size = fread(buffer, sizeof(char), buffer_size, from_file);
        i = cur_start = 0;
        is_blank = 1;
        while (i < buffer_size && i < copied_size) {
            if (i == buffer_size - 1) {
                fseek(from_file, cur_start - i - 1, SEEK_CUR);
            }
            else if (buffer[i] == '\n' || i == copied_size - 1) {
                if (!is_blank) {
                    written_size = fwrite(buffer + cur_start, sizeof(char), i - cur_start + 1, to_file);
                    if (written_size != i - cur_start + 1) {
                        fprintf(stderr, "Writing to file failed: %s\n", strerror(errno));
                        exit(1);
                    }
                }
                cur_start = i + 1;
                is_blank = 1;
            }
            else if (!isspace(buffer[i])) {
                is_blank = 0;
            }
            ++i;
        }
    } while (copied_size);

    return 0;
}

int main(int argc, char *argv[]) {
    char input_file_name[100] = {0};
    char output_file_name[100] = {0};
    if (argc == 2 || argc > 3) {
        fprintf(stderr, "Invalid number of arguments: expected 2\n");
        exit(1);
    }
    if (argc == 1) {
        printf("Input file to be copied:\n");
        scanf("%s", input_file_name);
        printf("Input name of file the contents will be copied to:\n");
        scanf("%s", output_file_name);
    }
    FILE *from_file, *to_file;
    if (argc == 3) {
        from_file = fopen(argv[1], "r");
        to_file = fopen(argv[2], "w");
    }
    else {
        from_file = fopen(input_file_name, "r");
        to_file = fopen(output_file_name, "w");
    }
    if (!from_file) {
        fprintf(stderr, "Input file could not be open or does not exist\n");
        exit(1);
    }
    if (!to_file) {
        fprintf(stderr, "Output file could not be created or overwritten\n");
        exit(1);
    }

    clock_t real_time_start, real_time_end;
    struct tms time_start, time_end;

    real_time_start = times(&time_start);
    copy_and_remove_blanks(from_file, to_file);
    real_time_end = times(&time_end);

    fclose(from_file);
    fclose(to_file);

    output_raport(real_time_start, real_time_end, &time_start, &time_end);

    return 0;
}
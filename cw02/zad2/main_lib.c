#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include "timing.h"

void count_chars_and_lines(char to_find, FILE *from_file) {
    size_t buffer_size = 1024;  // buffer size must be >= longest line length
    char buffer[buffer_size];
    
    int count_char, count_line, i, was_line_used;
    count_char = count_line = was_line_used = 0;

    size_t copied_size;
    do {
        copied_size = fread(buffer, sizeof(char), buffer_size, from_file);
        i = 0;
        while (i < copied_size && i < buffer_size) {
            if (buffer[i] == '\n') {
                was_line_used = 0;
            }
            if (buffer[i] == to_find) {
                ++count_char;
                if (!was_line_used) {
                    ++count_line;
                    was_line_used = 1;
                }
            }
            ++i;
        }
    } while (copied_size);

    printf("NUMBER OF \"%c\": %d\n", to_find, count_char);
    printf("NUMBER OF LINES CONTAINING \"%c\": %d\n\n", to_find, count_line);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Invalid number of arguments, expected 2\n");
        exit(1);
    }
    if (argv[1][1] != '\0') {
        fprintf(stderr, "Error: First argument must be a single character\n");
        exit(1);
    }

    FILE *from_file;
    from_file = fopen(argv[2], "r");
    if (!from_file) {
        fprintf(stderr, "Error: Input file could not be opened or does not exist\n");
        exit(1);
    }

    clock_t real_time_start, real_time_end;
    struct tms time_start, time_end;

    real_time_start = times(&time_start);
    count_chars_and_lines(argv[1][0], from_file);
    real_time_end = times(&time_end);

    fclose(from_file);

    output_raport(real_time_start, real_time_end, &time_start, &time_end);

    return 0;
}
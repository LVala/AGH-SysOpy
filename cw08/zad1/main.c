#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include "utils.h"

void manage_time(struct Times *times, int op, double *elapsed) {
    switch (op) {
        case START:
            clock_gettime(CLOCK_REALTIME, &times->start);
            break;
        case STOP:
            clock_gettime(CLOCK_REALTIME, &times->stop);
            *elapsed = (times->stop.tv_sec - times->start.tv_sec) + (double)(times->stop.tv_nsec - times->start.tv_nsec) / 1000000000L;
            break;
        default:
            ERROR(1, 0, "Error: unknown operation\n");
            *elapsed = -1;
    }
}

struct Image read_image(char *in_file) {
    FILE *fp = fopen(in_file, "r");
    if (fp == NULL) {
        ERROR(1, 1, "Error: input file could not be opened\n");
    }
    
    char buffer[MAX_LINE];
    int word_num = 0;

    struct Image image;

    while (fgets(buffer, MAX_LINE, fp) != NULL || word_num < 4) {
        char *saveptr;
        char *token = strtok_r(buffer, " \t\n", &saveptr);
        if (*token == '#') continue;
        while (token != NULL) {
            if (word_num == 0) {
                if (strcmp(token, "P2")) {
                ERROR(0, 0, "Warning: file does not begin with P2, ASCII PGM magic number, continuing reading...\n");
                }
                ++word_num;
            }
            else if (word_num == 1) {
                if ((image.width = strtol(token, NULL, 10)) == 0) {
                    ERROR(1, 1, "Error: invalid width in input file\n");
                }
                ++word_num;
            }
            else if (word_num == 2) {
                if ((image.height = strtol(token, NULL, 10)) == 0) {
                    ERROR(1, 1, "Error: invalid height in input file\n");
                }
                ++word_num;
                image.values = malloc(image.width*image.height*sizeof(int));
            }
            else if (word_num == 3) {
                if ((image.max_pixel = strtol(token, NULL, 10)) == 0) {
                    ERROR(1, 1, "Error: invalid maximum pixel value in input file\n");
                }
                ++word_num;
            }
            else {
                image.values[word_num-4] = strtol(token, NULL, 10);
                if (word_num-4 >= image.width*image.height) {
                    ERROR(1, 0, "Error: number of values exceeds width*height\n");
                }
                ++word_num;
            }
            token = strtok_r(NULL, " \t\n", &saveptr);
        }
    }

    fclose(fp);
    return image;
}

void save_image(char *out_file, struct Image image) {
    FILE *fp = fopen(out_file, "w");
    if (fp == NULL) {
        ERROR(1, 1, "Error: output file could not be opened\n");
    }
    if (fprintf(fp, "P2\n%d %d\n%d\n", image.width, image.height, image.max_pixel) == 0) {
        ERROR(1, 1, "Error: cannot write to output file\n");
    }

    char buffer[25];
    int cur_line_len = 0;

    for (int i=0; i<image.width*image.height; ++i) {
        sprintf(buffer, "%d", image.values[i]);
        if (cur_line_len + strlen(buffer) + 1 < MAX_LINE) {
            if (cur_line_len != 0) {
                if (fputc(' ', fp) == EOF) {
                    ERROR(1, 1, "Error: cannot write to output file\n");
                }
            }
            if (fputs(buffer, fp) == EOF) {
                ERROR(1, 1, "Error: cannot write to output file\n");
            }
            cur_line_len += strlen(buffer) + 1;
        }
        else {
            if (fputc('\n', fp) == EOF || fputs(buffer, fp) == EOF) {
                ERROR(1, 1, "Error: cannot write to output file\n");
            }
            cur_line_len = strlen(buffer) + 1;
        }
    }

    fclose(fp);
}

void *start_routine_numbers(void *arg) {
    struct Context *context = (struct Context *) arg;
    struct Image *image = context->image;
    struct Times times;
    manage_time(&times, START, NULL);

    for (int i=0; i<image->height*image->width; ++i) {
        int value = image->values[i] > image->max_pixel/2 ? image->max_pixel - image->values[i] : image->values[i];
        if (value % context->threads_num == context->index) {
            image->values[i] = image->max_pixel - image->values[i];
        }
    }

    double *thread_time = malloc(sizeof(double));
    manage_time(&times, STOP, thread_time);

    pthread_exit((void *) thread_time);
}

void *start_routine_block(void *arg) {
    struct Context *context = (struct Context *) arg;
    struct Image *image = context->image;
    struct Times times;
    manage_time(&times, START, NULL);

    int coef = ceil((double) image->width / context->threads_num);
    int left_end = context->index * coef;
    int right_end = (context->index + 1) * coef;
    for (int i=left_end; i<right_end && i<image->width; ++i) {
        for (int j=i; j<image->width*image->height; j += image->width) {
            image->values[j] = image->max_pixel - image->values[j];
        }
    }

    double *thread_time = malloc(sizeof(double));
    manage_time(&times, STOP, thread_time);

    pthread_exit((void *) thread_time);
}

void negate(struct Image image, int thread_num, int mode) {
    pthread_t *threads = calloc(thread_num, sizeof(pthread_t));
    struct Context *context = calloc(thread_num, sizeof(struct Context));
    struct Times times;
    manage_time(&times, START, NULL);

    for (int i=0; i<thread_num; ++i) {
        context[i].image = &image;
        context[i].index = i;
        context[i].threads_num = thread_num;
        int status;
        if (mode == NUMBERS) {
            status = pthread_create(&threads[i], NULL, start_routine_numbers, (void *) &context[i]);
        }
        else if (mode == BLOCK) {
            status = pthread_create(&threads[i], NULL, start_routine_block, (void *) &context[i]);
        }
        if (status != 0) {
            ERROR(1, 1, "Error: thread could not be created\n");
        }
    }

    for (int i=0; i<thread_num; ++i) {
        double *thread_time;
        pthread_join(threads[i], (void *) &thread_time);
        printf("THREAD %d: %.9f\n", i, *thread_time);
        free(thread_time);
    }

    double total_time;
    manage_time(&times, STOP, &total_time);
    printf("TOTAL TIME: %.9f\n", total_time);

    free(threads);
    free(context);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        ERROR(1,0,"Error: invalid number of arguments\n");
    }
    // arguments validation
    int thread_num = strtol(argv[1], NULL, 10);
    if (thread_num == 0) {
        ERROR(1, 0, "Error: first argument must be a positive number\n");
    }

    int mode;
    if (!strcmp(argv[2], "numbers")) {
        mode = NUMBERS;
    }
    else if (!strcmp(argv[2], "block")) {
        mode = BLOCK;
    }
    else {
        ERROR(1, 0, "Error: second argument must be either \"numbers\" or \"block\"\n");
    }
    char *in_file = argv[3];
    char *out_file = argv[4];

    struct Image image = read_image(in_file);

    negate(image, thread_num, mode);

    save_image(out_file, image);

    free(image.values);
    return 0;
}
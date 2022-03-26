#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/wait.h>

#ifndef RAPORT // raport name should be defined by a gcc flag
    #define RAPORT "raport.txt"
#endif

void output_raport(clock_t real_time_start, clock_t real_time_end, struct tms *time_start, struct tms *time_end) {

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

double f(double x) {
    return 4/(x*x + 1);
}

void calculate_integral(double width, int n_proc) {
    int n = 1/width;  // sum of widths is not always equal to the whole interval, thus this is an aproximation
    int iter_per_proc = n/n_proc + 1;  // +1 to ensure all of the rectangles can fit into n_proc files
    int i = 0;
    int num_of_file = 1;
    char filename[50];
    double result;

    while (num_of_file <= n_proc) {
        if (fork() == 0) {
            sprintf(filename, "w%d.txt", num_of_file);
            FILE *file = fopen(filename, "w");
            if (file == NULL) {
                fprintf(stderr, "Error while creating a file: %s\n", strerror(errno));
                exit(1);
            }

            for (int j = i; j < i + iter_per_proc && j < n; ++j) {
                result = width * f(j*width + (1/2)*width);
                fprintf(file, "%.15lf\n", result);
            }

            fclose(file);
            exit(0);
        }
        ++num_of_file;
        i += iter_per_proc;
    }

    while (wait(NULL) > 0);
}

double get_result(int n_proc) {
    char filename[50];
    double temp; 
    double result = 0;
    FILE *file;
    for (int i = 1; i<=n_proc; ++i) {
        sprintf(filename, "w%d.txt", i);
        file = fopen(filename, "r");
        while (fscanf(file, "%lf\n", &temp) != EOF) {
            result += temp;
        }
        fclose(file);
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: invalid number of arguments, expected 2\n");
        exit(1);
    }
    double width, result;
    int n_proc;

    width = strtof(argv[1], NULL);
    if (width == 0.0 || width > 1.0) {
        fprintf(stderr, "Error: first argument must be a double in range 0 excluded to 1 included\n");
        exit(1);
    }
    n_proc = strtol(argv[2], NULL, 10);
    if (n_proc == 0) {
        fprintf(stderr, "Error: second argument must be a positive integer\n");
        exit(1);
    }

    clock_t real_time_start, real_time_end;
    struct tms time_start, time_end;
    real_time_start = times(&time_start);

    calculate_integral(width, n_proc);
    result = get_result(n_proc);
    printf("THE RESULT IS %.15f\n", result);
    
    real_time_end = times(&time_end);
    output_raport(real_time_start, real_time_end, &time_start, &time_end);

    return 0;
}
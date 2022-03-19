#include <sys/times.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timing.h"

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
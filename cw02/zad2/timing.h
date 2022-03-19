#ifndef TIMING
#define TIMING

#include <sys/times.h>

void output_raport(clock_t, clock_t, struct tms*, struct tms*);

#endif
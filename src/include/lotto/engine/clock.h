#ifndef LOTTO_ENGINE_CLOCK_H
#define LOTTO_ENGINE_CLOCK_H

#include <stdint.h>
#include <time.h>

#define NS_PER_INST   1
#define SLEEP_DIVISOR 10

// slow down time for CLOCK_DRIFT>1
// speed up  time for CLOCK_DRIFT<1
#define CLOCK_DRIFT 0.9

#define NSEC_IN_USEC 1000ULL
#define NSEC_IN_MSEC 1000000UL
#define NSEC_IN_SEC  1000000000ULL

#define USEC_IN_MSEC 1000ULL
#define USEC_IN_SEC  1000000ULL

#define MSEC_IN_SEC 1000ULL

#define INST_PER_SEC (NSEC_IN_SEC / NSEC_PER_INST)

#define ICOUNT_OFFSET_INC 51

uint64_t clock_ns(void);
void clock_time(struct timespec *ts);
void clock_print(void);
void lotto_clock_set(const struct timespec *ts);
int timespec_compare(const struct timespec *ts1, const struct timespec *ts2);

#endif

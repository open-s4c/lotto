#ifndef LOTTO_MODULES_CLOCK_H
#define LOTTO_MODULES_CLOCK_H

#include <stdint.h>
#include <time.h>

#define NS_PER_INST   1
#define SLEEP_DIVISOR 10

#define CLOCK_DRIFT 0.9

#define NSEC_IN_USEC 1000ULL
#define NSEC_IN_MSEC 1000000UL
#define NSEC_IN_SEC  1000000000ULL

#define USEC_IN_MSEC 1000ULL
#define USEC_IN_SEC  1000000ULL

#define MSEC_IN_SEC 1000ULL

#define INST_PER_SEC (NSEC_IN_SEC / NS_PER_INST)

// Read the current emulated Lotto time in nanoseconds. This is the public
// semantic clock query and therefore goes through the clock event path.
uint64_t lotto_clock_read(void);

// Read the current emulated Lotto time as a timespec from the local clock
// module state. This does not publish EVENT_CLOCK_READ and is safe from
// inside sequencer handlers.
void lotto_clock_time(struct timespec *ts);

// Advance the emulated Lotto clock to at least the requested timestamp.
// Intended for internal module use, primarily timeout handling.
void lotto_clock_leap(const struct timespec *ts);

// Debug helper to print the current emulated Lotto time.
void lotto_clock_print(void);

static inline int
timespec_compare(const struct timespec *ts1, const struct timespec *ts2)
{
    return ts1->tv_sec < ts2->tv_sec   ? -1 :
           ts1->tv_sec > ts2->tv_sec   ? 1 :
           ts1->tv_nsec < ts2->tv_nsec ? -1 :
           ts1->tv_nsec > ts2->tv_nsec ? 1 :
                                         0;
}

#endif

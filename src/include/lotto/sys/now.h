/*
 */
#ifndef LOTTO_NOW_H
#define LOTTO_NOW_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/time.h>
#include <lotto/util/casts.h>

typedef uint64_t nanosec_t;

#define NOW_NANOSECOND  (CAST_TYPE(uint64_t, 1))
#define NOW_MICROSECOND (1000 * NOW_NANOSECOND)
#define NOW_MILLISECOND (1000 * NOW_MICROSECOND)
#define NOW_SECOND      (1000 * NOW_MILLISECOND)


static inline nanosec_t
now(void)
{
    struct timespec ts;

    if (sys_clock_gettime(CLOCK_MONOTONIC, &ts)) {
        log_infof("ERROR: could not get clock time\n");
        exit(-1);
    }

    const nanosec_t seconds_ns = (CAST_TYPE(uint64_t, ts.tv_sec)) * NOW_SECOND;
    const nanosec_t nanoseconds_ns = CAST_TYPE(uint64_t, ts.tv_nsec);
    const nanosec_t result_ns      = seconds_ns + nanoseconds_ns;

    return result_ns;
}

static inline double
in_sec(nanosec_t ts)
{
    return ((double)ts) / NOW_SECOND;
}

static inline struct timespec
to_timespec(nanosec_t ts)
{
    return (struct timespec){
        .tv_sec  = CAST_TYPE(int32_t, (ts / NOW_SECOND)),
        .tv_nsec = CAST_TYPE(int32_t, (ts % NOW_SECOND)),
    };
}

#endif

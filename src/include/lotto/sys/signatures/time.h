#ifndef LOTTO_SIGNATURES_TIME_H
#define LOTTO_SIGNATURES_TIME_H

#include <time.h>

#include <lotto/sys/signatures/defaults_head.h>
#include <sys/time.h>
#include <sys/types.h>

#define SYS_TIME  SYS_FUNC(LIBC, return, SIG(time_t, time, time_t *, tloc), )
#define SYS_CLOCK SYS_FUNC(LIBC, return, SIG(clock_t, clock, void), )
#define SYS_CLOCK_GETTIME                                                      \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, clock_gettime, clockid_t, clockid, struct timespec *, tp), )
#define SYS_GETTIMEOFDAY                                                       \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, gettimeofday, struct timeval *, tv, void *, tz), )
#define SYS_NANOSLEEP                                                          \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, nanosleep, const struct timespec *, req,                 \
                 struct timespec *, rem), )

#define FOR_EACH_SYS_TIME_WRAPPED                                              \
    SYS_TIME                                                                   \
    SYS_CLOCK                                                                  \
    SYS_CLOCK_GETTIME                                                          \
    SYS_GETTIMEOFDAY                                                           \
    SYS_NANOSLEEP

#define FOR_EACH_SYS_TIME_CUSTOM

#define FOR_EACH_SYS_TIME                                                      \
    FOR_EACH_SYS_TIME_WRAPPED                                                  \
    FOR_EACH_SYS_TIME_CUSTOM

#endif // LOTTO_SIGNATURES_TIME_H

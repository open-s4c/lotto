#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include <dice/module.h>
#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/clock.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <sys/timeb.h>
#include <sys/types.h>

static void
_clock_timespec_from_read(struct timespec *ts)
{
    uint64_t cur_ns = lotto_clock_read();
    uint64_t sec    = cur_ns / NSEC_IN_SEC;
    uint64_t nsec   = cur_ns % NSEC_IN_SEC;

    ASSERT(sec < LONG_MAX);
    ts->tv_sec = (long)sec;
    ASSERT(nsec < LONG_MAX);
    ts->tv_nsec = (long)nsec;
}

int
sched_setaffinity(__pid_t pid, size_t s, const cpu_set_t *cset)
{
    logger_debugf("warning: ignoring sched_setaffinity call\n");
    return 0;
}

#if !defined(QLOTTO_ENABLED)
pid_t
fork(void)
{
    logger_fatalf("error: fork call\n");
    return 0;
}
#endif

time_t
real_time(time_t *tloc)
{
    REAL_DECL(time_t, time, time_t *tloc);
    if (REAL_NAME(time) == NULL)
        REAL_NAME(time) = real_func("time", 0);

    return REAL(time, tloc);
}

time_t
time(time_t *tloc)
{
    if (!time_config()->enabled) {
        return real_time(tloc);
    }

    uint64_t cur_ns = lotto_clock_read();
    uint64_t sec    = cur_ns / NSEC_IN_SEC;
    ASSERT(sec <= LONG_MAX);
    if (NULL != tloc) {
        *tloc = (long)sec;
    }

    return (long)sec;
}

int
gettimeofday(struct timeval *tv, void *tz)
{
    if (!time_config()->enabled) {
        REAL_INIT(int, gettimeofday, struct timeval *, void *);
        return REAL(gettimeofday, tv, tz);
    }

    struct timespec ts;
    _clock_timespec_from_read(&ts);

    tv->tv_sec   = ts.tv_sec;
    uint64_t sec = ts.tv_nsec / NSEC_IN_SEC;
    ASSERT(sec <= LONG_MAX);
    tv->tv_usec = (long)sec;
    return 0;
}

int
clock_gettime(clockid_t clockid, struct timespec *tp)
{
    if (!time_config()->enabled) {
        REAL_INIT(int, clock_gettime, clockid_t, struct timespec *);
        return REAL(clock_gettime, clockid, tp);
    }

    _clock_timespec_from_read(tp);
    return 0;
}

clock_t
clock(void)
{
    if (!time_config()->enabled) {
        REAL_INIT(clock_t, clock, void);
        return REAL(clock);
    }

    uint64_t cur_ns = lotto_clock_read();
    uint64_t ret    = (cur_ns * CLOCKS_PER_SEC) / NSEC_IN_SEC;
    ASSERT(ret <= LONG_MAX);
    return (long)ret;
}

int
ftime(struct timeb *tp)
{
    if (!time_config()->enabled) {
        REAL_INIT(int, ftime, struct timeb *);
        return REAL(ftime, tp);
    }

    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        return -1;
    }

    tp->time    = ts.tv_sec;
    tp->millitm = (unsigned short)(ts.tv_nsec / NSEC_IN_MSEC);

    return 0;
}

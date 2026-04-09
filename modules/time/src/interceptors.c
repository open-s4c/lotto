#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/recorder.h>
#include <lotto/modules/time/events.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <lotto/yield.h>
#include <sys/types.h>

typedef struct {
    const char *func;
} time_yield_event_t;

static inline void
intercept_time_yield(const char *func)
{
    time_yield_event_t ev = {.func = func};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_TIME_YIELD, &ev, 0);
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
    ASSERT(0);
    return 0;
}
#endif

time_t
real_time(time_t *tloc)
{
    REAL_DECL(time_t, time, time_t * tloc);
    if (REAL_NAME(time) == NULL)
        REAL_NAME(time) = real_func("time", 0);

    return REAL(time, tloc);
}

time_t
time(time_t *tloc)
{
    uint64_t cur_ns = clock_ns();
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
    struct timespec ts;
    clock_time(&ts);

    tv->tv_sec   = ts.tv_sec;
    uint64_t sec = ts.tv_nsec / NSEC_IN_SEC;
    ASSERT(sec <= LONG_MAX);
    tv->tv_usec = (long)sec;
    return 0;
}

int
clock_gettime(clockid_t clockid, struct timespec *tp)
{
    clock_time(tp);
    return 0;
}

clock_t
clock(void)
{
    uint64_t cur_ns = clock_ns();
    uint64_t ret    = (cur_ns * CLOCKS_PER_SEC) / NSEC_IN_SEC;
    ASSERT(ret <= LONG_MAX);
    return (long)ret;
}


#if defined(QLOTTO_ENABLED)
int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    uint64_t cur_ns   = clock_ns();
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((req->tv_sec * NSEC_IN_SEC + req->tv_nsec) /
                                  (SLEEP_DIVISOR));

    while (cur_ns < ns_end) {
        intercept_time_yield("nanosleep");
        cur_ns = clock_ns();
    }
    return 0;
}

int
usleep(useconds_t usec)
{
    uint64_t cur_ns   = clock_ns();
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((usec * NSEC_IN_USEC) / (SLEEP_DIVISOR));

    while (cur_ns < ns_end) {
        intercept_time_yield("usleep");
        cur_ns = clock_ns();
    }
    return 0;
}

unsigned int
sleep(unsigned int seconds)
{
    uint64_t cur_ns   = clock_ns();
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((seconds * NSEC_IN_SEC) / (SLEEP_DIVISOR));

    while (cur_ns < ns_end) {
        intercept_time_yield("sleep");
        cur_ns = clock_ns();
    }
    return 0;
}
#else
int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    return lotto_yield(0);
}

int
usleep(useconds_t usec)
{
    return lotto_yield(0);
}

unsigned int
sleep(unsigned int seconds)
{
    return lotto_yield(0);
}
#endif

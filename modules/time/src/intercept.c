#include <alloca.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/callrec.h>
#include <lotto/base/record.h>
#include <lotto/base/topic.h>
#include <lotto/base/trace.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/recorder.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>
#include <lotto/sys/sched.h>
#include <lotto/unsafe/time.h>
#include <lotto/util/casts.h>
#include <sys/types.h>

int
sched_setaffinity(__pid_t pid, size_t s, const cpu_set_t *cset)
{
    logger_debugf("warning: ignoring sched_setaffinity call\n");
    return 0;
}

/**
 * TODO: fork is also intercepted if --before-run executes a script that
 * executes some other processes like cp. Needs to be fixed to only intercept
 * for the SUT process.
 */
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
    uint64_t cur_ns = clock_ns();

    context_t ctx;
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((req->tv_sec * NSEC_IN_SEC + req->tv_nsec) /
                                  (SLEEP_DIVISOR));

    ctx.cat    = CAT_SYS_YIELD;
    ctx.icount = 0;
    ctx.func   = "nanosleep";

    while (cur_ns < ns_end) {
        intercept_capture(&ctx);
        cur_ns = clock_ns();
    }
    return 0;
}

int
usleep(useconds_t usec)
{
    uint64_t cur_ns = clock_ns();

    context_t ctx;
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((usec * NSEC_IN_USEC) / (SLEEP_DIVISOR));

    ctx.cat    = CAT_SYS_YIELD;
    ctx.icount = 0;
    ctx.func   = "usleep";

    while (cur_ns < ns_end) {
        intercept_capture(&ctx);
        cur_ns = clock_ns();
    }
    return 0;
}

unsigned int
sleep(unsigned int seconds)
{
    uint64_t cur_ns = clock_ns();

    context_t ctx;
    uint64_t ns_start = cur_ns;
    uint64_t ns_end   = ns_start + ((seconds * NSEC_IN_SEC) / (SLEEP_DIVISOR));

    ctx.cat    = CAT_SYS_YIELD;
    ctx.icount = 0;
    ctx.func   = "sleep";

    while (cur_ns < ns_end) {
        intercept_capture(&ctx);
        cur_ns = clock_ns();
    }
    return 0;
}
#else
int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    return 0;
}

int
usleep(useconds_t usec)
{
    return 0;
}

unsigned int
sleep(unsigned int seconds)
{
    return 0;
}
#endif

ssize_t
getrandom(void *buf, size_t buflen, unsigned int flags)
{
    if (buflen == 0)
        return 0;

    ASSERT(buf != NULL);

    uint32_t buf_idx = 0;
    uint8_t *buf_p   = (uint8_t *)buf;

    uint64_t seed = prng_next();
    uint64_t r    = 0;

    while (buf_idx < buflen) {
        if (buf_idx % 8 == 0) {
            r = lcg_next(seed);
        }
        buf_p[buf_idx] = CAST_TYPE(uint8_t, r & 0xFF);
        r              = r >> 8;
        buf_idx++;
    }

    return buf_idx;
}

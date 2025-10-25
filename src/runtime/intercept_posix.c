#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/real.h>

static inline void
capture_simple(const char *func, category_t cat, arg_t arg0)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = func;
        c->cat       = cat;
        c->args[0]   = arg0;
        intercept_capture(c);
    }
}

int
sched_yield(void)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = __FUNCTION__;
        c->cat       = CAT_USER_YIELD;
        intercept_capture(c);
    }

    REAL_LIBC_INIT(int, sched_yield, void);
    if (REAL_NAME(sched_yield) != NULL) {
        return REAL(sched_yield);
    }
    return 0;
}

#if !defined(__APPLE__)
int
sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_CALL;
    c->args[0]   = arg(pid_t, pid);
    c->args[1]   = arg(size_t, cpusetsize);
    c->args[2]   = arg_ptr(mask);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_LIBC_INIT(int, sched_setaffinity, pid_t, size_t, const cpu_set_t *);
    int ret = 0;
    if (REAL_NAME(sched_setaffinity) != NULL) {
        ret = REAL(sched_setaffinity, pid, cpusetsize, mask);
    }

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}
#endif

int
lotto_yield(bool advisory)
{
    capture_simple(__FUNCTION__,
                   advisory ? CAT_SYS_YIELD : CAT_USER_YIELD,
                   arg(bool, advisory));
    return sched_yield();
}

time_t
time(time_t *tloc)
{
    REAL_LIBC_INIT(time_t, time, time_t *);
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_CALL;
    c->args[0]   = arg_ptr(tloc);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    time_t ret = 0;
    if (REAL_NAME(time) != NULL) {
        ret = REAL(time, tloc);
    } else {
        REAL_LIBC_INIT(int, clock_gettime, clockid_t, struct timespec *);
        if (REAL_NAME(clock_gettime) != NULL) {
            struct timespec ts = {0};
            if (REAL(clock_gettime, CLOCK_REALTIME, &ts) == 0) {
                ret = (time_t)ts.tv_sec;
                if (tloc != NULL) {
                    *tloc = ret;
                }
            }
        } else if (tloc != NULL) {
            *tloc = 0;
        }
    }

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
gettimeofday(struct timeval *tv, void *tz)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_CALL;
    c->args[0]   = arg_ptr(tv);
    c->args[1]   = arg_ptr(tz);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_LIBC_INIT(int, gettimeofday, struct timeval *, void *);
    int ret = 0;
    if (REAL_NAME(gettimeofday) != NULL) {
        ret = REAL(gettimeofday, tv, tz);
    } else {
        REAL_LIBC_INIT(int, clock_gettime, clockid_t, struct timespec *);
        if (REAL_NAME(clock_gettime) != NULL) {
            struct timespec ts = {0};
            if (REAL(clock_gettime, CLOCK_REALTIME, &ts) == 0 && tv != NULL) {
                tv->tv_sec  = ts.tv_sec;
                tv->tv_usec = (suseconds_t)(ts.tv_nsec / 1000);
            }
        } else if (tv != NULL) {
            tv->tv_sec  = 0;
            tv->tv_usec = 0;
        }
    }

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
clock_gettime(clockid_t clockid, struct timespec *tp)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_CALL;
    c->args[0]   = arg(clockid_t, clockid);
    c->args[1]   = arg_ptr(tp);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_LIBC_INIT(int, clock_gettime, clockid_t, struct timespec *);
    int ret = 0;
    if (REAL_NAME(clock_gettime) != NULL) {
        ret = REAL(clock_gettime, clockid, tp);
    } else {
        ret = -1;
    }

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

clock_t
clock(void)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = __FUNCTION__;
        c->cat       = CAT_CALL;
        intercept_capture(c);
    }

    REAL_LIBC_INIT(clock_t, clock, void);
    if (REAL_NAME(clock) != NULL) {
        return REAL(clock);
    }
    return 0;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_POLL;
    c->args[0]   = arg_ptr(fds);
    c->args[1]   = arg(nfds_t, nfds);
    c->args[2]   = arg(int, timeout);
    if (intercept_capture != NULL) {
        intercept_capture(c);
    }

    REAL_LIBC_INIT(int, poll, struct pollfd *, nfds_t, int);
    if (REAL_NAME(poll) != NULL) {
        return REAL(poll, fds, nfds, timeout);
    }
    return 0;
}

int
ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts,
      const sigset_t *sigmask)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_POLL;
    c->args[0]   = arg_ptr(fds);
    c->args[1]   = arg(nfds_t, nfds);
    c->args[2]   = arg_ptr(timeout_ts);
    c->args[3]   = arg_ptr(sigmask);
    if (intercept_capture != NULL) {
        intercept_capture(c);
    }

    REAL_LIBC_INIT(int, ppoll, struct pollfd *, nfds_t,
                   const struct timespec *, const sigset_t *);
    if (REAL_NAME(ppoll) != NULL) {
        return REAL(ppoll, fds, nfds, timeout_ts, sigmask);
    }

    int timeout_ms = -1;
    if (timeout_ts != NULL) {
        timeout_ms = (int)(timeout_ts->tv_sec * 1000 +
                           timeout_ts->tv_nsec / 1000000);
    }
    return poll(fds, nfds, timeout_ms);
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    context_t *c = ctx();
    c->func      = __FUNCTION__;
    c->cat       = CAT_SYS_YIELD;
    c->args[0]   = arg_ptr(req);
    c->args[1]   = arg_ptr(rem);
    if (intercept_before_call != NULL) {
        intercept_before_call(c);
    }

    REAL_LIBC_INIT(int, nanosleep, const struct timespec *, struct timespec *);
    int ret = 0;
    if (REAL_NAME(nanosleep) != NULL) {
        ret = REAL(nanosleep, req, rem);
    }

    if (intercept_after_call != NULL) {
        intercept_after_call(__FUNCTION__);
    }
    return ret;
}

int
usleep(useconds_t usec)
{
    capture_simple(__FUNCTION__, CAT_SYS_YIELD, arg(useconds_t, usec));
    REAL_LIBC_INIT(int, usleep, useconds_t);
    if (REAL_NAME(usleep) != NULL) {
        return REAL(usleep, usec);
    }
    struct timespec ts = {
        .tv_sec  = usec / 1000000,
        .tv_nsec = (long)(usec % 1000000) * 1000,
    };
    return nanosleep(&ts, NULL);
}

unsigned int
sleep(unsigned int seconds)
{
    capture_simple(__FUNCTION__, CAT_SYS_YIELD, arg(unsigned int, seconds));
    REAL_LIBC_INIT(unsigned int, sleep, unsigned int);
    if (REAL_NAME(sleep) != NULL) {
        return REAL(sleep, seconds);
    }
    struct timespec ts = {.tv_sec = seconds, .tv_nsec = 0};
    nanosleep(&ts, NULL);
    return 0;
}

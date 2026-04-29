#include <time.h>
#include <unistd.h>

#include "state.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/sleep/events.h>

static inline void
intercept_sleep_yield(const char *func, const struct timespec *duration)
{
    sleep_yield_event_t ev = {.func = func, .duration = *duration};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SLEEP_YIELD, &ev, 0);
}

static int
sleep_yield_for(const char *func, const struct timespec *req)
{
    intercept_sleep_yield(func, req);
    return 0;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    return sleep_yield_for("nanosleep", req);
}

int
usleep(useconds_t usec)
{
    struct timespec req = {
        .tv_sec  = usec / USEC_IN_SEC,
        .tv_nsec = (usec % USEC_IN_SEC) * NSEC_IN_USEC,
    };
    return sleep_yield_for("usleep", &req);
}

unsigned int
sleep(unsigned int seconds)
{
    struct timespec req = {
        .tv_sec  = seconds,
        .tv_nsec = 0,
    };
    (void)sleep_yield_for("sleep", &req);
    return 0;
}

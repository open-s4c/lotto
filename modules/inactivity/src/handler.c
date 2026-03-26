#include <limits.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/logger.h>

static struct event_data {
    task_id id;
    uint64_t alarm;
} _event_data;
LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {
    const capture_point *cp = (const capture_point *)md;
    if (cp) {
        _event_data.id = cp->id;
    }
})

static timer_t timer = 0;

static void _reset_alarm(timer_t timer_id);

static void
_expired(union sigval timer_data)
{
    struct event_data *data = timer_data.sival_ptr;
    logger_println("Task [%lu] has no capture point received after %lu seconds",
                   data->id, data->alarm);
    _reset_alarm(timer);
    data->alarm += inactivity_config()->alarm;
}

static timer_t
_set_alarm(const capture_point *cp)
{
    timer_t timer_id;
    struct sigevent sev = {0};
    _event_data.alarm   = inactivity_config()->alarm;
    _event_data.id      = cp->id;

    sev.sigev_notify          = SIGEV_THREAD;
    sev.sigev_notify_function = &_expired;
    sev.sigev_value.sival_ptr = &_event_data;

    ASSERT(inactivity_config()->alarm <= LONG_MAX);
    struct itimerspec its = {.it_value.tv_sec =
                                 (long)inactivity_config()->alarm,
                             .it_value.tv_nsec    = 0,
                             .it_interval.tv_sec  = 0,
                             .it_interval.tv_nsec = 0};

    int res = 0;
    res     = timer_create(CLOCK_REALTIME, &sev, &timer_id);
    ASSERT(res == 0);

    res = timer_settime(timer_id, 0, &its, NULL);
    ASSERT(res == 0);

    return timer_id;
}

static void
_reset_alarm(timer_t timer_id)
{
    ASSERT(inactivity_config()->alarm <= LONG_MAX);
    struct itimerspec its = {.it_value.tv_sec =
                                 (long)inactivity_config()->alarm,
                             .it_value.tv_nsec    = 0,
                             .it_interval.tv_sec  = 0,
                             .it_interval.tv_nsec = 0};
    int res               = timer_settime(timer_id, 0, &its, NULL);
    ASSERT(res == 0);
}

/*******************************************************************************
 * handler functions
 ******************************************************************************/
STATIC void
_inactivity_alarm(const capture_point *cp, event_t *e)
{
    (void)e;
    if (inactivity_config()->alarm != 0) {
        if (timer != 0) {
            timer_delete(timer);
        }
        timer = _set_alarm(cp);
    }
}

ON_SEQUENCER_CAPTURE(_inactivity_alarm)

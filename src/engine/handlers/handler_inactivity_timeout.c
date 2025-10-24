/*
 */

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/inactivity_timeout.h>
#include <lotto/sys/logger_block.h>

static struct event_data {
    task_id id;
    uint64_t alarm;
} _event_data;
PS_SUBSCRIBE_INTERFACE(TOPIC_NEXT_TASK, {
    const context_t *ctx = as_any(v);
    if (ctx) {
        _event_data.id = ctx->id;
    }
})

static timer_t timer = 0;

static void _reset_alarm(timer_t timer_id);

static void
_expired(union sigval timer_data)
{
    struct event_data *data = timer_data.sival_ptr;
    log_println("Task [%lu] has no capture point received after %lu seconds",
                data->id, data->alarm);
    _reset_alarm(timer);
    data->alarm += inactivity_timeout_config()->alarm;
}

static timer_t
_set_alarm(const context_t *ctx)
{
    timer_t timer_id;
    struct sigevent sev = {0};
    _event_data.alarm   = inactivity_timeout_config()->alarm;
    _event_data.id      = ctx->id;

    sev.sigev_notify          = SIGEV_THREAD;
    sev.sigev_notify_function = &_expired;
    sev.sigev_value.sival_ptr = &_event_data;

    ASSERT(inactivity_timeout_config()->alarm <= LONG_MAX);
    struct itimerspec its = {.it_value.tv_sec =
                                 (long)inactivity_timeout_config()->alarm,
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
    ASSERT(inactivity_timeout_config()->alarm <= LONG_MAX);
    struct itimerspec its = {.it_value.tv_sec =
                                 (long)inactivity_timeout_config()->alarm,
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
_inactivity_alarm(const context_t *ctx, event_t *e)
{
    if (inactivity_timeout_config()->alarm != 0) {
        if (timer != 0) {
            timer_delete(timer);
        }
        timer = _set_alarm(ctx);
    }
}

REGISTER_HANDLER(SLOT_INACTIVITY_TIMEOUT, _inactivity_alarm)

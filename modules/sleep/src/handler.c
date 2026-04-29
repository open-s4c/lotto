#include "state.h"
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/tidset.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/sleep/events.h>
#include <lotto/modules/timeout/timeout.h>

static tidset_t _waiters;

REGISTER_STATE(EPHEMERAL, _waiters, { tidset_init(&_waiters); })

LOTTO_SUBSCRIBE(EVENT_TIMEOUT_TRIGGER, {
    task_id id = as_uval(v);
    if (tidset_has(&_waiters, id)) {
        tidset_remove(&_waiters, id);
    }
    return PS_OK;
})

static bool
_should_wait(task_id id)
{
    return tidset_has(&_waiters, id);
}

static struct timespec
_deadline_from_duration(const struct timespec *duration)
{
    struct timespec deadline;
    lotto_clock_time(&deadline);

    uint64_t duration_ns =
        ((uint64_t)duration->tv_sec * NSEC_IN_SEC + duration->tv_nsec) /
        SLEEP_DIVISOR;
    uint64_t deadline_ns = (uint64_t)deadline.tv_nsec + duration_ns;
    deadline.tv_sec += (long)(deadline_ns / NSEC_IN_SEC);
    deadline.tv_nsec = (long)(deadline_ns % NSEC_IN_SEC);
    return deadline;
}

static bool
_is_zero_duration(const struct timespec *duration)
{
    return duration->tv_sec == 0 && duration->tv_nsec == 0;
}

STATIC void
_sleep_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || cp->type_id != EVENT_SLEEP_YIELD) {
        return;
    }

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    if (!e->readonly && !e->is_chpt) {
        e->reason  = REASON_SYS_YIELD;
        e->is_chpt = true;
    }

    if (sleep_config()->mode != SLEEP_MODE_UNTIL) {
        return;
    }

    sleep_yield_event_t *ev = CP_PAYLOAD(ev);
    if (_is_zero_duration(&ev->duration)) {
        return;
    }

    struct timespec deadline = _deadline_from_duration(&ev->duration);
    handler_timeout_register(cp->id, &deadline);
    tidset_insert(&_waiters, cp->id);
    tidset_remove(&e->tset, cp->id);

    ASSERT(e->any_task_filter == NULL);
    e->any_task_filter = _should_wait;
}
ON_SEQUENCER_CAPTURE(_sleep_handle)

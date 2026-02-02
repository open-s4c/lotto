#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <dice/module.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/clock.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/modules/timeout.h>

/*******************************************************************************
 * state
 ******************************************************************************/
typedef struct timeout {
    tiditem_t ti;
    struct timespec deadline;
} timeout_t;

tidmap_t _state;

REGISTER_STATE(EPHEMERAL, _state,
               { tidmap_init(&_state, MARSHABLE_STATIC(sizeof(timeout_t))); })

/*******************************************************************************
 * utils
 ******************************************************************************/
static void
_trigger_timeout(tidset_t *tset, task_id id)
{
    struct value val = uval(id);
    PS_PUBLISH_INTERFACE(TOPIC_TRIGGER_TIMEOUT, val);
    bool ret = tidset_insert(tset, id);
    ASSERT(ret);
}

static void
_check_deadlines(tidset_t *tset)
{
    struct timespec now;
    clock_time(&now);
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;) {
        const timeout_t *timeout = (const timeout_t *)cur;
        if (timespec_compare(&timeout->deadline, &now) > 0) {
            cur = tidmap_next(cur);
            continue;
        }
        task_id id = cur->key;
        _trigger_timeout(tset, id);
        cur = tidmap_next(cur);
        tidmap_deregister(&_state, id);
    }
}

static void
_time_leap(tidset_t *tset)
{
    const timeout_t *min = &(timeout_t){.deadline.tv_sec = INT32_MAX};
    bool multiple        = false;
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;
         cur                  = tidmap_next(cur)) {
        const timeout_t *timeout = (const timeout_t *)cur;
        int cmp = timespec_compare(&timeout->deadline, &min->deadline);
        if (cmp > 0) {
            continue;
        }
        multiple = min->ti.key != NO_TASK && cmp == 0;
        min      = timeout;
    }
    task_id id = min->ti.key;
    ASSERT(id != NO_TASK);
    lotto_clock_set(&min->deadline);
    if (multiple) {
        _check_deadlines(tset);
        return;
    }
    _trigger_timeout(tset, id);
    tidmap_deregister(&_state, id);
}

STATIC void
_timeout_handle(const context_t *ctx, event_t *e)
{
    if (tidmap_size(&_state) == 0) {
        return;
    }
    _check_deadlines(&e->tset);
    if (tidset_size(&e->tset) > 0) {
        return;
    }
    _time_leap(&e->tset);
}
REGISTER_HANDLER(SLOT_TIMEOUT, _timeout_handle);

/*******************************************************************************
 * interface
 ******************************************************************************/
void
handler_timeout_register(task_id id, const struct timespec *deadline)
{
    timeout_t *timeout = (timeout_t *)tidmap_register(&_state, id);
    struct timespec now;
    clock_time(&now);
    ASSERT(timeout);
    timeout->deadline = *deadline;
}

void
handler_timeout_deregister(task_id id)
{
    tidmap_deregister(&_state, id);
}

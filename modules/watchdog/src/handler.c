#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/prng.h>
#include <lotto/engine/sequencer.h>
#include <lotto/modules/yield/context_payload.h>
#include <lotto/runtime/memaccess_payload.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

/*******************************************************************************
 * @file handler_watchdog.c
 *
 * The watchdog observes if the current task produces too many capture points in
 * a row. If that occurs, the watchdog forces the current capture point into a
 * change point.
 ******************************************************************************/

/*******************************************************************************
 * watchdog implementation
 ******************************************************************************/
typedef struct {
    task_id id;
    nanosec_t ts;
    clk_t ticks;
} state_t;
static state_t _state;

static void
_watchdog_reset(task_id id)
{
    _state.id = id;
    _state.ticks =
        prng_range(watchdog_config()->budget / 2, watchdog_config()->budget);
}

static bool
_watchdog_ok(task_id id)
{
    if (id != _state.id) {
        _watchdog_reset(id);
        return true;
    }

    if (--_state.ticks == 0) {
        _watchdog_reset(id);
        return false;
    }

    return true;
}

/*******************************************************************************
 * handler function
 ******************************************************************************/
STATIC void
_watchdog_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp && cp->id != NO_TASK);
    ASSERT(e);
    if (e->skip || !watchdog_config()->enabled)
        return;

    if (e->readonly)
        return;

    switch (cp->src_type) {
        case EVENT_MA_AREAD:
        case EVENT_MA_AWRITE:
        case EVENT_MA_XCHG:
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_RMW:
        case EVENT_MA_FENCE:
            if (cp->src_chain != CAPTURE_AFTER)
                break;
            // fallthru
        case EVENT_MA_READ:
        case EVENT_MA_WRITE:

            /* Typically, we do not reschedule when performing
             * non-atomic reads or writes. However, to avoid tasks stuck
             * in spinloops, we limit for how long a task may run
             * uninterrupted. */
            if (_watchdog_ok(cp->id))
                break;

            /* Avoid repeating current task: remove it from the tset. */
            tidset_remove(&e->tset, cp->id);
            if (!e->is_chpt) {
                e->reason  = REASON_WATCHDOG;
                e->is_chpt = true;
            }
            e->selector = SELECTOR_RANDOM;
            break;

        default:
            if (context_yield_event(cp) != CONTEXT_YIELD_USER) {
                break;
            }
            if (_watchdog_ok(cp->id))
                break;
            tidset_remove(&e->tset, cp->id);
            if (!e->is_chpt) {
                e->reason  = REASON_WATCHDOG;
                e->is_chpt = true;
            }
            e->selector = SELECTOR_RANDOM;
            break;
    }
}
REGISTER_SEQUENCER_HANDLER(_watchdog_handle)

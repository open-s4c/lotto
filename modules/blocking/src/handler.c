#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/blocking/blocking.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

#define BLOCKED_TASKS(replay)                                                  \
    ((replay) ? &_state.blocked_replay : &_state.blocked_actual)

/*******************************************************************************
 * ephemeral state
 ******************************************************************************/

typedef struct {
    tidset_t blocked_actual;
    tidset_t blocked_replay;
    task_id prev;
    bool replaying;
    bool consumed;
} state_t;

static state_t _state;

REGISTER_EPHEMERAL(_state, {
    tidset_init(&_state.blocked_actual);
    tidset_init(&_state.blocked_replay);
})
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_PERSISTENT,
                { _state.consumed = false; })

/*******************************************************************************
 * handler
 ******************************************************************************/

STATIC void
_blocking_handle(const capture_point *cp, event_t *e)
{
    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);
    ASSERT(e);

    /* unblock tasks if any requested */
    tidset_subtract(&_state.blocked_actual, &e->unblocked);

    /* handle replay */
    if (e->replay) {
        _state.replaying = true;
        if (_state.consumed) {
            tidset_clear(returned_tasks());
        } else {
            tidset_subtract(&_state.blocked_replay, returned_tasks());
            _state.consumed = true;
        }
        tidset_remove(&_state.blocked_replay, cp->id);
    } else {
        if (_state.replaying) {
            _state.replaying = false;
            tidset_copy(returned_tasks(), &_state.blocked_replay);
            tidset_subtract(returned_tasks(), &_state.blocked_actual);
            tidset_intersect(&_state.blocked_actual, &_state.blocked_replay);
        } else {
            tidset_copy(returned_tasks(), &e->unblocked);
        }
    }

    e->should_record |= tidset_size(returned_tasks()) > 0 &&
                        !(tidset_size(returned_tasks()) == 1 &&
                          tidset_has(returned_tasks(), cp->id));

    /* When blocking, the current task should not be selectable. */
    if (cp->blocking && cp->src_type != EVENT_TASK_CREATE) {
        ASSERT(!e->is_chpt);
        ASSERT(!tidset_has(&_state.blocked_actual, cp->id));

        tidset_insert(&_state.blocked_actual, cp->id);
        if (e->replay) {
            tidset_insert(&_state.blocked_replay, cp->id);
        }
        e->reason  = REASON_CALL;
        e->is_chpt = true;
    }

    _state.prev = cp->id;

    if (e->skip)
        return;

    /* filter blocked tasks */
    tidset_subtract(&e->tset, BLOCKED_TASKS(e->replay));
}
REGISTER_SEQUENCER_HANDLER(_blocking_handle)

/*******************************************************************************
 * debug functions
 ******************************************************************************/

bool
_lotto_is_blocked(task_id id, bool replay)
{
    return tidset_has(BLOCKED_TASKS(replay), id);
}

void
_lotto_print_returned()
{
    marshable_print(&returned_tasks()->m);
}

bool
any_blocked()
{
    return tidset_size(BLOCKED_TASKS(_state.replaying)) > 0;
}

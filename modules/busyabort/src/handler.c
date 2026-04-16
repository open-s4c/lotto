#include "state.h"
#include <dice/events/memaccess.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>

/*
 * Abort when the same task repeats the same memaccess event at the same pc
 * for too long, which usually indicates a stuck busy-loop.
 */
typedef struct {
    task_id last_tid;
    type_id last_type_id;
    uintptr_t last_pc;
    uint64_t repeat_count;
} busy_state_t;

static busy_state_t _state;

STATIC void
_busyabort_handle(const capture_point *cp, event_t *e)
{
    (void)e;
    ASSERT(cp != NULL);

    if (!busyabort_config()->enabled) {
        return;
    }
    if (!is_memaccess(cp->type_id)) {
        return;
    }
    if (busyabort_config()->threshold == 0) {
        return;
    }

    if (_state.last_tid == cp->id && _state.last_type_id == cp->type_id &&
        _state.last_pc == cp->pc) {
        _state.repeat_count++;
    } else {
        _state.last_tid     = cp->id;
        _state.last_type_id = cp->type_id;
        _state.last_pc      = cp->pc;
        _state.repeat_count = 1;
    }

    if (_state.repeat_count < busyabort_config()->threshold) {
        return;
    }

    logger_fatalf(
        "[tid: %lu, type: %s, pc: %p, count: %lu] stuck at busyloop\n", cp->id,
        ps_type_str(cp->type_id), (void *)cp->pc, _state.repeat_count);
}
ON_SEQUENCER_CAPTURE(_busyabort_handle)

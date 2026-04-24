#include <stdint.h>
#include <stdlib.h>

#include <lotto/base/reason.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/termination/events.h>
#include <lotto/modules/termination/state.h>
#include <lotto/util/macros.h>

static uint64_t cur_chpt    = 0;
static uint64_t cur_preempt = 0;
static task_id last         = 0;

STATIC void
_termination_handle(const capture_point *cp, event_t *e)
{
    switch (cp->type_id) {
        case EVENT_TERMINATE_SUCCESS:
            e->reason = REASON_SUCCESS;
            return;
        case EVENT_TERMINATE_FAIL:
            e->reason = REASON_ASSERT_FAIL;
            return;
        case EVENT_TERMINATE_STOP:
            e->reason = REASON_SUCCESS;
            return;
        default:
            break;
    }

    if (IS_REASON_SHUTDOWN(e->reason)) {
        return;
    }
    if (e->is_chpt) {
        cur_chpt++;
    }
    if (e->next != last && last > 0) {
        cur_preempt++;
    }
    switch (termination_config()->mode) {
        case TERMINATION_MODE_CLK:
            if (e->clk > termination_config()->limit) {
                e->reason = REASON_SHUTDOWN;
            }
            break;
        case TERMINATION_MODE_CHPT:
            if (cur_chpt > termination_config()->limit) {
                e->reason = REASON_SHUTDOWN;
            }
            break;
        case TERMINATION_MODE_PREEMPT:
            if (cur_preempt > termination_config()->limit) {
                e->reason = REASON_SHUTDOWN;
            }
            break;
        case TERMINATION_MODE_TIME:
            if (CAST_TYPE(uint64_t, time(NULL)) > termination_config()->limit) {
                e->reason = REASON_SHUTDOWN;
            }
            break;
        case TERMINATION_MODE_REPLAY:
            if (!e->replay) {
                e->reason = REASON_SHUTDOWN;
            }

        default:
            break;
    }
}
ON_SEQUENCER_CAPTURE(_termination_handle);

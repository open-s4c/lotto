/*
 */

#include <stdint.h>
#include <stdlib.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/base/reason.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/termination.h>
#include <lotto/util/macros.h>

static uint64_t cur_chpt    = 0;
static uint64_t cur_preempt = 0;
static task_id last         = 0;

STATIC void
_termination_handle(const context_t *ctx, event_t *e)
{
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
REGISTER_HANDLER(SLOT_TERMINATION, _termination_handle);

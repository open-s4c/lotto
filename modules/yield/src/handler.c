#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/sequencer.h>
#include <lotto/modules/yield/context_payload.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

STATIC void
_yield_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !yield_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    switch (context_yield_event(cp)) {
        case CONTEXT_YIELD_USER:
            if (!e->readonly && !e->is_chpt) {
                if (!yield_config()->advisory) {
                    tidset_remove(&e->tset, cp->id);
                }
                if (!e->is_chpt) {
                    e->reason  = REASON_USER_YIELD;
                    e->is_chpt = true;
                }
            }
            break;
        case CONTEXT_YIELD_SYS:
            if (!e->readonly && !e->is_chpt) {
                e->reason  = REASON_SYS_YIELD;
                e->is_chpt = true;
            }
            break;
        default:
            break;
    }
}
REGISTER_SEQUENCER_HANDLER(_yield_handle)

#include "state.h"
#include <lotto/engine/sequencer.h>
#include <lotto/modules/yield/events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

STATIC void
_yield_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !yield_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    switch (cp->type_id) {
        case EVENT_YIELD_SCHED:
        case EVENT_YIELD_USER:
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
        case EVENT_YIELD_SYS:
            if (!e->readonly && !e->is_chpt) {
                e->reason  = REASON_SYS_YIELD;
                e->is_chpt = true;
            }
            break;
        default:
            break;
    }
}
ON_SEQUENCER_CAPTURE(_yield_handle)

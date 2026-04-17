#include "state.h"
#include <dice/events/memaccess.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

STATIC void
_memaccess_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !memaccess_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    if (e->readonly)
        return;

    switch (cp->chain_id) {
        case CHAIN_INGRESS_BEFORE:
            switch (cp->type_id) {
                case EVENT_MA_AREAD:
                case EVENT_MA_AWRITE:
                case EVENT_MA_XCHG:
                case EVENT_MA_CMPXCHG:
                case EVENT_MA_RMW:
                case EVENT_MA_FENCE:
                    if (!e->is_chpt) {
                        e->reason  = REASON_DETERMINISTIC;
                        e->is_chpt = true;
                    }
                    break;
                default:
                    break;
            }
            break;

        case CHAIN_INGRESS_AFTER:
            /* _AFTER memaccess events are informative: the operation has
             * already
             * happened, so handlers may observe its result, but the sequencer
             * should not treat this point as a scheduling decision. Keep it
             * non-change-point and readonly. */
            switch (cp->type_id) {
                case EVENT_MA_AREAD:
                case EVENT_MA_AWRITE:
                case EVENT_MA_XCHG:
                case EVENT_MA_CMPXCHG:
                case EVENT_MA_RMW:
                case EVENT_MA_FENCE:
                    if (!e->is_chpt) {
                        e->is_chpt  = false;
                        e->readonly = true;
                    }
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}
ON_SEQUENCER_CAPTURE(_memaccess_handle)

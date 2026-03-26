#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include "state.h"
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/memaccess_payload.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

STATIC void
_atomic_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !atomic_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    if (e->readonly)
        return;

    switch (cp->src_chain) {
        case CHAIN_INGRESS_BEFORE:
            switch (cp->src_type) {
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

        default:
            break;
    }
}
REGISTER_SEQUENCER_HANDLER(_atomic_handle)

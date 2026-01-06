#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/atomic.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

STATIC void
_atomic_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !atomic_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);

    if (e->readonly)
        return;

    // NOLINTBEGIN(bugprone-branch-clone): Fixme - Fix and remove no lint line
    switch (ctx->cat) {
        case CAT_BEFORE_AREAD:
        case CAT_BEFORE_AWRITE:
        case CAT_BEFORE_XCHG:
        case CAT_BEFORE_CMPXCHG:
        case CAT_BEFORE_RMW:
        case CAT_BEFORE_FENCE:
            if (!e->is_chpt) {
                e->reason  = REASON_DETERMINISTIC;
                e->is_chpt = true;
            }
            break;

        case CAT_AFTER_AREAD:
        case CAT_AFTER_AWRITE:
        case CAT_AFTER_XCHG:
        case CAT_AFTER_RMW:
        case CAT_AFTER_CMPXCHG_S:
        case CAT_AFTER_CMPXCHG_F:
        case CAT_AFTER_FENCE:
        default:
            break;
            // NOLINTEND(bugprone-branch-clone)
    }
}
REGISTER_HANDLER(SLOT_ATOMIC, _atomic_handle)

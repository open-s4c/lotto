#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/yield.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

STATIC void
_yield_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !yield_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);


    switch (ctx->cat) {
        case CAT_USER_YIELD:
            if (!e->readonly && !e->is_chpt) {
                if (!yield_config()->advisory) {
                    tidset_remove(&e->tset, ctx->id);
                }
                if (!e->is_chpt) {
                    e->reason  = REASON_USER_YIELD;
                    e->is_chpt = true;
                }
            }
            break;
        case CAT_SYS_YIELD:
            if (!e->readonly && !e->is_chpt) {
                e->reason  = REASON_SYS_YIELD;
                e->is_chpt = true;
            }
            break;
        default:
            break;
    }
}
REGISTER_HANDLER(SLOT_YIELD, _yield_handle)

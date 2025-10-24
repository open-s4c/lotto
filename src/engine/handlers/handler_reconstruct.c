/*
 */
#define LOG_PREFIX LOG_CUR_FILE

#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/reconstruct.h>
#include <lotto/util/macros.h>

static void
_check_log_before(const context_t *ctx, event_t *e)
{
    e->should_record = true;
    if (reconstruct_config()->tid != ctx->id) {
        tidset_remove(&e->tset, ctx->id);
        if (!e->is_chpt) {
            e->reason  = REASON_DETERMINISTIC;
            e->is_chpt = true;
        }
        e->selector = SELECTOR_RANDOM;
    }
}

static void
_check_log_after(const context_t *ctx, event_t *e)
{
    e->should_record = true;

    reconstruct_get_log()->tid  = ctx->id;
    reconstruct_get_log()->id   = ctx->args[0].value.u64;
    reconstruct_get_log()->data = ctx->args[1].value.u64;

    if (reconstruct_config()->enabled && !e->replay) {
        e->reason = REASON_IGNORE;
        ASSERT(tidset_has(&e->tset, ctx->id));
        ASSERT(e->next == NO_TASK);
        e->next = ctx->id;
        if (reconstruct_config()->tid != NO_TASK) {
            log_infof("\n[reconstruct handler] clk: %lu\n", e->clk);
            log_infof("Looking for: [%lu, %lu, 0x%lx]\n",
                      reconstruct_config()->tid, reconstruct_config()->id,
                      reconstruct_config()->data);
            log_infof("Found:       [%lu, %lu, 0x%lx]\n",
                      reconstruct_get_log()->tid, reconstruct_get_log()->id,
                      reconstruct_get_log()->data);
            log_infof("\n");
        }
    }
}

STATIC void
_reconstruct_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->readonly || e->skip)
        return;

    ASSERT(ctx);
    reconstruct_get_log()->tid = NO_TASK;

    switch (ctx->cat) {
        case CAT_LOG_BEFORE:
            if (reconstruct_config()->enabled) {
                _check_log_before(ctx, e);
            }
            break;
        case CAT_LOG_AFTER:
            _check_log_after(ctx, e);
            break;
        default:
            break;
    }
}
REGISTER_HANDLER(SLOT_RECONSTRUCT, _reconstruct_handle)

/*
 */
#include <limits.h>
#include <string.h>

/******************************************************************************
 * @file handler_ichpt.c
 * @brief Dynamically convert instructions into change points.
 ******************************************************************************/

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/handlers/ichpt.h>
#include <lotto/states/handlers/ichpt.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

/* *****************************************************************************
 * Upon start, we load the bag of addresses (in `_initial`). When the program
 * ends, we save the addresses in _final. When running/stressing, we load the
 * final addresses of the previous run into the intial of the current run.
 * ****************************************************************************/
PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    bag_clear(ichpt_final());
    bag_copy(ichpt_final(), ichpt_initial());
})

/* *****************************************************************************
 * public interface
 * ****************************************************************************/
size_t
ichpt_count()
{
    return bag_size(ichpt_final());
}

bool
is_ichpt(uintptr_t a)
{
    stable_address_t sa =
        stable_address_get(a, sequencer_config()->stable_address_method);
    const bagitem_t *it;
    for (it = bag_iterate(ichpt_final()); it; it = bag_next(it)) {
        const item_t *i = (const item_t *)it;
        if (stable_address_equals(&i->addr, &sa))
            return true;
    }
    return false;
}

void
add_ichpt(uintptr_t a)
{
    if (is_ichpt(a))
        return;
    item_t *i = (item_t *)bag_add(ichpt_final());
    i->addr = stable_address_get(a, sequencer_config()->stable_address_method);
}

void
ichpt_reset()
{
    bag_clear(ichpt_final());
}

void
it_is_ichpt()
{
}

/* ****************************************************************************
 * handler
 * ****************************************************************************/
STATIC void
_ichpt_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !ichpt_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);

    uint64_t tid = ctx->id;
    if (ctx->vid)
        tid = ctx->vid;

    if (e->readonly)
        return;

    if (is_ichpt(ctx->pc)) {
        it_is_ichpt();
        log_infof("[%lx] instruction change point at pc %p\n", tid,
                  (void *)ctx->pc);
        e->is_chpt = true;
    }
}
REGISTER_HANDLER(SLOT_ICHPT, _ichpt_handle)

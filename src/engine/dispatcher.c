/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>

static struct {
    handle_f handle;
} _slots[SLOT_END_];

static slot_t _last;

void
dispatcher_register(slot_t slot, handle_f handle)
{
    log_debugf("slot: %s\n", slot_str(slot));
    ASSERT(slot < SLOT_END_);
    ASSERT(handle != NULL && "empty registrations are not allowed");
    ASSERT(_slots[slot].handle == NULL &&
           "repeated registrations are not allowed");
    _slots[slot].handle = handle;

    if (slot > _last)
        _last = slot;
}

static void
_handle_chain(const context_t *ctx, event_t *e)
{
    handle_f handle;
    for (slot_t slot = 0; slot <= _last && !e->skip; slot++) {
        if ((handle = _slots[slot].handle) != NULL)
            handle(ctx, e);
    }
}

task_id
dispatch_event(const context_t *ctx, event_t *e)
{
    /* dispatch to handlers */
    _handle_chain(ctx, e);

    if (!e->is_chpt) {
        ASSERT(e->next == NO_TASK || e->next == ctx->id);
        return ctx->id;
    }

    /* Decision has been made by some handler, comply */
    if (e->next != NO_TASK) {
        return e->next;
    }

    /* If no task left in tidset, pick any */
    if (tidset_size(&e->tset) == 0) {
        return ANY_TASK;
    }

    /* we need to select the next task from the remaining tasks */
    if (e->selector == SELECTOR_UNDEFINED)
        e->selector = SELECTOR_RANDOM; // comes from config

    switch (e->selector) {
        case SELECTOR_RANDOM:
            if (e->reason == REASON_UNKNOWN)
                e->reason = REASON_DETERMINISTIC;
            return tidset_get(&e->tset, prng_range(0, tidset_size(&e->tset)));
        case SELECTOR_FIRST:
            return tidset_get(&e->tset, 0);

        default:
            log_debugf("Selector %d\n", e->selector);
    }
    ASSERT(0);
    return NO_TASK;
}

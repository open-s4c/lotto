#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <string.h>
#include <lotto/base/strategy.h>
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/events.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/state.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>

LOTTO_ADVERTISE_TYPE(EVENT_ENGINE__CAPTURE)

void handle_creation(const context_t *ctx, event_t *e);

static int
_task_id_cmp(const void *a, const void *b)
{
    const task_id *ta = a;
    const task_id *tb = b;

    return *ta > *tb ? 1 :
           *ta < *tb ? -1 :
                       0;
}

task_id
dispatch_event(const context_t *ctx, event_t *e)
{
    handle_creation(ctx, e);

    /* dispatch to handlers in slot order through the capture chain */
    PS_PUBLISH(CHAIN_LOTTO_DEFAULT, EVENT_ENGINE__CAPTURE, e,
               (struct metadata *)ctx);

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
    if (e->selector == SELECTOR_UNDEFINED) {
        if (strcmp(sequencer_config()->strategy, strategy_str(STRATEGY_FIRST)) ==
            0) {
            tidset_sort(&e->tset, _task_id_cmp);
            e->selector = SELECTOR_FIRST;
            if (e->reason == REASON_UNKNOWN)
                e->reason = REASON_DETERMINISTIC;
        } else {
            e->selector = SELECTOR_RANDOM; // comes from config
        }
    }

    switch (e->selector) {
        case SELECTOR_RANDOM:
            if (e->reason == REASON_UNKNOWN)
                e->reason = REASON_DETERMINISTIC;
            return tidset_get(&e->tset, prng_range(0, tidset_size(&e->tset)));
        case SELECTOR_FIRST:
            return tidset_get(&e->tset, 0);

        default:
            logger_debugf("Selector %d\n", e->selector);
    }
    ASSERT(0);
    return NO_TASK;
}

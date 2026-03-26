#include <limits.h>
#include <string.h>

/******************************************************************************
 * @file handler_ichpt.c
 * @brief Dynamically convert instructions into change points.
 ******************************************************************************/

#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/modules/ichpt/ichpt.h>
#include <lotto/modules/ichpt/state.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

/* *****************************************************************************
 * Upon start, we load the bag of addresses (in `_initial`). When the program
 * ends, we save the addresses in _final. When running/stressing, we load the
 * final addresses of the previous run into the intial of the current run.
 * ****************************************************************************/
LOTTO_SUBSCRIBE(EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG, {
    vec_union(ichpt_final(), ichpt_initial(), ichpt_item_compare);
})

/* *****************************************************************************
 * public interface
 * ****************************************************************************/
size_t
ichpt_count()
{
    return vec_size(ichpt_final());
}

bool
is_ichpt(uintptr_t a)
{
    stable_address_t sa =
        stable_address_get(a, sequencer_config()->stable_address_method);
    for (size_t i = 0; i < vec_size(ichpt_final()); ++i) {
        const item_t *it = (const item_t *)vec_get(ichpt_final(), i);
        if (stable_address_equals(&it->addr, &sa)) {
            return true;
        }
    }
    return false;
}

void
add_ichpt(uintptr_t a)
{
    if (is_ichpt(a))
        return;
    item_t *i = (item_t *)vec_add(ichpt_final());
    i->addr = stable_address_get(a, sequencer_config()->stable_address_method);
}

void
ichpt_reset()
{
    vec_clear(ichpt_final());
}

void
it_is_ichpt()
{
}

/* ****************************************************************************
 * handler
 * ****************************************************************************/
STATIC void
_ichpt_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !ichpt_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);

    uint64_t tid = cp->id;
    if (cp->vid)
        tid = cp->vid;

    if (e->readonly)
        return;

    if (is_ichpt(cp->pc)) {
        it_is_ichpt();
        logger_infof("[%lx] instruction change point at pc %p\n", tid,
                     (void *)cp->pc);
        e->is_chpt = true;
    }
}
ON_SEQUENCER_CAPTURE(_ichpt_handle)

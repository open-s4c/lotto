/**
 * - lotto_task_velocity(probability) is intercepted changing the execution
 * probability for a current task
 * - If the event is immutable, the handler doesn't filter out anything
 * - If the event is mutable, the handler remove the given task from a tidset
 * according to the probability
 */
#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include "state.h"
#include <lotto/base/tidmap.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/task_velocity/events.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>
#include <lotto/velocity.h>

#define NAME "TASK VELOCITY"
#define SLOT SLOT_TASK_VELOCITY

typedef struct {
    tiditem_t t;
    uint64_t probability;
} task_t;

typedef struct {
    marshable_t m;
    tidmap_t map;
} state_t;

static state_t _state;

STATIC void _task_velocity_print(const marshable_t *m);
REGISTER_EPHEMERAL(_state, {
    tidmap_init(&_state.map, MARSHABLE_STATIC(sizeof(task_t)));
    _state.m.print = _task_velocity_print;
})

/*******************************************************************************
 * internal functions
 ******************************************************************************/
STATIC void
_task_velocity_print(const marshable_t *m)
{
    const state_t *s = (const state_t *)m;
    logger_printf("prob   = [");
    const tiditem_t *cur = tidmap_iterate(&s->map);
    bool first           = true;
    for (; cur; cur = tidmap_next(cur)) {
        if (!first)
            logger_printf(", ");
        first     = false;
        task_t *t = (task_t *)cur;
        logger_printf("%lu:%lu", cur->key, t->probability);
    }
    logger_println("]");
}

static bool
_filter_by_probability(task_id task)
{
    task_t *t = (task_t *)tidmap_find(&_state.map, task);
    ASSERT(t);
    uint64_t filter_value =
        prng_range(LOTTO_TASK_VELOCITY_MIN, LOTTO_TASK_VELOCITY_MAX);
    return t->probability >= filter_value;
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_task_velocity_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !task_velocity_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);
    task_t *t = NULL;
    if (cp->src_type == EVENT_TASK_FINI) {
        tidmap_deregister(&_state.map, cp->id);
        ASSERT(!tidset_has(&e->tset, cp->id));
    } else if (cp->src_type == EVENT_TASK_INIT) {
        t = (task_t *)tidmap_find(&_state.map, cp->id);
        ASSERT(t == NULL);
        t = (task_t *)tidmap_register(&_state.map, cp->id);
        ASSERT(t);
        t->probability = LOTTO_TASK_VELOCITY_MAX;
    } else if (cp->src_type == EVENT_TASK_VELOCITY) {
        t = (task_t *)tidmap_find(&_state.map, cp->id);
        ASSERT(t);
        t->probability = (uint64_t)((task_velocity_event_t *)cp->payload)->probability;
    }
    if (e->readonly || e->skip) {
        return;
    }

    tidset_filter(&e->tset, _filter_by_probability);
    e->is_chpt |= !tidset_has(&e->tset, cp->id);
}
REGISTER_SEQUENCER_HANDLER(_task_velocity_handle)

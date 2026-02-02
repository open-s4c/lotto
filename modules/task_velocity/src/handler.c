/**
 * - lotto_task_velocity(probability) is intercepted changing the execution
 * probability for a current task
 * - If the event is immutable, the handler doesn't filter out anything
 * - If the event is mutable, the handler remove the given task from a tidset
 * according to the probability
 */
#define LOGGER_BLOCK LOGGER_CUR_BLOCK

#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/modules/task_velocity/state.h>
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
_task_velocity_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !task_velocity_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    task_t *t = NULL;
    switch (ctx->cat) {
        case CAT_TASK_FINI:
            tidmap_deregister(&_state.map, ctx->id);
            ASSERT(!tidset_has(&e->tset, ctx->id));
            break;

        case CAT_TASK_INIT:
            t = (task_t *)tidmap_find(&_state.map, ctx->id);
            ASSERT(t == NULL);
            t = (task_t *)tidmap_register(&_state.map, ctx->id);
            ASSERT(t);
            t->probability = LOTTO_TASK_VELOCITY_MAX;
            break;
        case CAT_TASK_VELOCITY:
            t = (task_t *)tidmap_find(&_state.map, ctx->id);
            ASSERT(t);
            t->probability = CAST_TYPE(uint64_t, ctx->args[0].value.u64);
            break;
        default:
            break;
    }
    if (e->readonly || e->skip) {
        return;
    }

    tidset_filter(&e->tset, _filter_by_probability);
    e->is_chpt |= !tidset_has(&e->tset, ctx->id);
}
REGISTER_HANDLER(SLOT_TASK_VELOCITY, _task_velocity_handle)

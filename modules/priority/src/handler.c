/**
 * - When a thead is created it gets priority 0
 * - __lotto_priority(false, prio) is ignored
 * - __lotto_priority(true, prio) is intercepted and the task gets his
 * priority updated to prio
 * - If the event is immutable, the handler doesn't filter out anything
 * - If the event is mutable, the handler filters out all tasks whose priority
 * is not maximum
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include "category.h"
#include "state.h"
#include <dice/module.h>
#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

static category_t CAT_PRIORITY;

typedef struct {
    tiditem_t t;
    int64_t priority;
} task_t;

typedef struct {
    marshable_t m;
    tidmap_t map;
} state_t;

static state_t _state;

STATIC void _priority_print(const marshable_t *m);
REGISTER_EPHEMERAL(_state, {
    tidmap_init(&_state.map, MARSHABLE_STATIC(sizeof(task_t)));
    _state.m.print = _priority_print;
})

/*******************************************************************************
 * internal functions
 ******************************************************************************/
STATIC void
_priority_print(const marshable_t *m)
{
    const state_t *s = (const state_t *)m;
    logger_infof("prio   = [");
    const tiditem_t *cur = tidmap_iterate(&s->map);
    bool first           = true;
    for (; cur; cur = tidmap_next(cur)) {
        if (!first)
            logger_printf(", ");
        first     = false;
        task_t *t = (task_t *)cur;
        logger_printf("%lu:%ld", cur->key, t->priority);
    }
    logger_println("]");
}

static int64_t _max_priority;

static bool
_is_max_priority(task_id task)
{
    task_t *t = (task_t *)tidmap_find(&_state.map, task);
    ASSERT(t);
    return t->priority == _max_priority;
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_priority_handle(const context_t *ctx, event_t *e)
{
    once(CAT_PRIORITY = priority_category());
    ASSERT(e);
    if (e->skip || !priority_config()->enabled)
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
            t->priority = 0;
            break;
        default:
            if (ctx->cat != CAT_PRIORITY) {
                break;
            }
            t = (task_t *)tidmap_find(&_state.map, ctx->id);
            ASSERT(t);
            t->priority = (int64_t)ctx->args[0].value.u64;
            break;
    }
    if (e->readonly || e->skip) {
        return;
    }
    _max_priority = INT64_MIN;
    for (size_t i = 0; i < e->tset.size; i++) {
        t = (task_t *)tidmap_find(&_state.map, e->tset.tasks[i]);
        ASSERT(t);
        _max_priority =
            _max_priority < t->priority ? t->priority : _max_priority;
    }
    tidset_filter(&e->tset, _is_max_priority);
    e->is_chpt |= !tidset_has(&e->tset, ctx->id);
}
REGISTER_HANDLER(SLOT_PRIORITY, _priority_handle)

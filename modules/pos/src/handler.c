/*******************************************************************************
 * Implements POS
 *
 * http://www.cs.columbia.edu/~junfeng/papers/pos-cav18.pdf
 ******************************************************************************/
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/tidmap.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/modules/pos/state.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

/*******************************************************************************
 * state
 ******************************************************************************/

typedef struct {
    tiditem_t t;
    uint64_t priority;
    uintptr_t addr;
    bool is_write;
} task_t;
#define MARSHABLE_TASK MARSHABLE_STATIC(sizeof(task_t))
STATIC void _pos_print(const marshable_t *m);
static tidmap_t _state;
REGISTER_EPHEMERAL(_state, {
    tidmap_init(&_state, MARSHABLE_TASK);
    _state.m.print = _pos_print;
})

static uint64_t
_fresh_priority(uint64_t rval)
{
    return rval;
}

static int
_pos_cmp(const void *a, const void *b)
{
    const task_id *ta = (const task_id *)a;
    const task_id *tb = (const task_id *)b;

    const task_t *tta = (const task_t *)tidmap_find(&_state, *ta);
    const task_t *ttb = (const task_t *)tidmap_find(&_state, *tb);
    ASSERT(tta);
    ASSERT(ttb);

    return ttb->priority > tta->priority ? 1 :
           ttb->priority < tta->priority ? -1 :
           *ta > *tb                     ? 1 :
           *ta < *tb                     ? -1 :
                                           0;
}

static void
_pos_sort(tidset_t *tset)
{
    tidset_sort(tset, _pos_cmp);
}

static void
_reset_races(task_id id)
{
    task_t *t = (task_t *)tidmap_find(&_state, id);
    ASSERT(t);
    if (!t->addr) {
        return;
    }
    bool has_race = false;
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur && !has_race;
         cur                  = tidmap_next(cur)) {
        task_t *other = (task_t *)cur;
        if (t == other || t->addr != other->addr) {
            continue;
        }
        has_race = t->is_write || other->is_write;
    }
    if (!has_race) {
        return;
    }
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;
         cur                  = tidmap_next(cur)) {
        task_t *other = (task_t *)cur;
        if (t == other || t->addr != other->addr) {
            continue;
        }
        other->priority = _fresh_priority(prng_next());
    }
}

static void
_reset_wd()
{
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;
         cur                  = tidmap_next(cur)) {
        task_t *t = (task_t *)cur;
        t->priority *= pos_config()->wd_divisor;
    }
}

/*******************************************************************************
 * handler
 ******************************************************************************/
STATIC void
_pos_handle(const context_t *ctx, event_t *e)
{
    once({
        pos_config()->enabled =
            (strcmp(sequencer_config()->strategy, "pos") == 0);

        const char *var          = getenv("LOTTO_POS_WD_THRESHOLD");
        pos_config()->wd_divisor = var ? strtoull(var, NULL, 10) : 10;
        pos_config()->wd_threshold =
            pos_config()->wd_threshold / pos_config()->wd_divisor;
    });
    ASSERT(e);
    if (e->skip)
        return;

    ASSERT(ctx);

    uintptr_t addr = 0;
    bool is_write  = false;
    task_t *t      = NULL;

    switch (ctx->cat) {
        case CAT_TASK_FINI:
            tidmap_deregister(&_state, ctx->id);
            ASSERT(!tidset_has(&e->tset, ctx->id));
            break;

        case CAT_TASK_INIT:
            t = (task_t *)tidmap_find(&_state, ctx->id);
            ASSERT(t == NULL);
            t = (task_t *)tidmap_register(&_state, ctx->id);
            ASSERT(t);
            t->is_write = false;
            t->addr     = 0;
            t->priority = _fresh_priority(prng_next());
            break;

        case CAT_BEFORE_WRITE:
        case CAT_BEFORE_AWRITE:
        case CAT_BEFORE_CMPXCHG:
        case CAT_BEFORE_XCHG:
        case CAT_BEFORE_RMW:
            is_write = true;
            addr     = ctx->args[0].value.ptr;
            break;

        case CAT_BEFORE_AREAD:
        case CAT_BEFORE_READ:
            addr = ctx->args[0].value.ptr;

        default:
            break;
    }

    if (!pos_config()->enabled || !e->is_chpt ||
        e->selector != SELECTOR_UNDEFINED || e->readonly ||
        tidset_size(&e->tset) <= 1) {
        return;
    }

    if ((t = (task_t *)tidmap_find(&_state, ctx->id))) {
        t->is_write = is_write;
        t->addr     = addr;
        if (t->priority < pos_config()->wd_threshold) {
            _reset_wd();
        } else {
            t->priority = _fresh_priority(prng_next());
        }
    } else {
        ASSERT(ctx->cat == CAT_TASK_FINI);
    }
    _pos_sort(&e->tset);
    _reset_races(tidset_get(&e->tset, 0));
    e->selector = SELECTOR_FIRST;
    e->readonly = true;
    e->reason   = REASON_DETERMINISTIC;
}
REGISTER_HANDLER(SLOT_POS, _pos_handle);

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
STATIC void
_pos_print(const marshable_t *m)
{
    const tidmap_t *s = (const tidmap_t *)m;
    logger_infof("available events (tid, prio, addr, is_write) = [");
    const tiditem_t *cur = tidmap_iterate(s);
    bool first           = true;
    for (; cur; cur = tidmap_next(cur)) {
        if (!first)
            logger_printf(", ");
        first     = false;
        task_t *t = (task_t *)cur;
        logger_printf("(%lu, %lu, %lu, %s)", cur->key, t->priority, t->addr,
                      t->is_write ? "true" : "false");
    }
    logger_println("]");
}

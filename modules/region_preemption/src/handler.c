/*
 */
#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/states/handlers/region_preemption.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/macros.h>

/*******************************************************************************
 * state
 ******************************************************************************/
typedef struct {
    tiditem_t t;
    uint64_t counter;
} task_t;
static tidmap_t _state;

STATIC void _region_preemption_print(const marshable_t *m);

REGISTER_EPHEMERAL(_state, {
    tidmap_init(&_state, MARSHABLE_STATIC(sizeof(task_t)));
    _state.m.print = _region_preemption_print;
})

/*******************************************************************************
 * handler functions
 ******************************************************************************/
static task_id _task;

static void
_exit_region(task_id tid)
{
    task_t *t = (task_t *)tidmap_find(&_state, tid);
    ASSERT(t);
    ASSERT(t->counter > 0);
    t->counter--;
    if (!t->counter) {
        tidmap_deregister(&_state, tid);
    }
    if (_task == tid) {
        _task = NO_TASK;
    }
}

static void
_exit_task(task_id tid)
{
    tidmap_deregister(&_state, tid);
    if (_task == tid) {
        _task = NO_TASK;
    }
}

static void
_enter_region()
{
    if (_task == NO_TASK) {
        return;
    }
    task_t *t = (task_t *)tidmap_find(&_state, _task);
    if (!t) {
        t = (task_t *)tidmap_register(&_state, _task);
    }
    ASSERT(t);
    t->counter++;
    _task = NO_TASK;
}

static bool _warned;
static bool _warn;

void
_region_preemption_warning(task_id tid)
{
    if (_warned) {
        return;
    }
    _warned = true;
    logger_errorf("atomicity violated by task %lu\n", tid);
    logger_errorf("atomic task(s):");
    bool first = true;
    for (const tiditem_t *cur = tidmap_iterate(&_state); cur;
         cur                  = tidmap_next(cur)) {
        if (!first) {
            logger_printf(",");
        }
        first = false;
        logger_printf(" %lu", cur->key);
    }
    logger_printf("\n");
    logger_errorf("this error indicates either of:\n");
    logger_errorf(
        "- region misuse (creating new tasks or blocking inside the region)\n");
    logger_errorf(
        "- Lotto misconfiguration (the region handler being executed too "
        "late)\n");
    logger_errorf("- unhandled concurrency\n");
    logger_errorf("- internal bug\n");
    logger_errorf("to debug set a breakpoint on %s\n", __FUNCTION__);
}

static void
_region_check(task_id tid)
{
    if (!_warn || !region_preemption_config()->default_on ||
        tidmap_size(&_state) == 0) {
        return;
    }
    task_t *t = (task_t *)tidmap_find(&_state, tid);
    if (t == NULL) {
        _region_preemption_warning(tid);
    }
}

STATIC void
_region_preemption_handle(const context_t *ctx, event_t *e)
{
    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    ASSERT(e);

    if (!region_preemption_config()->enabled) {
        return;
    }

    _region_check(ctx->id);

    switch (ctx->cat) {
        case CAT_REGION_PREEMPTION:
            if (ctx->args[0].value.u64) {
                break;
            }
            _exit_region(ctx->id);
            break;
        case CAT_TASK_FINI:
            _exit_task(ctx->id);
            break;

        default:
            break;
    }

    bool in_region = tidmap_find(&_state, ctx->id) != NULL;

    if ((_warn = !e->readonly && e->selector == SELECTOR_UNDEFINED &&
                 in_region == region_preemption_config()->default_on &&
                 tidset_has(&e->tset, ctx->id) && !e->skip)) {
        e->selector = SELECTOR_FIRST;
        e->readonly = true;
        tidset_make_first(&e->tset, ctx->id);
    }

    _enter_region();

    switch (ctx->cat) {
        case CAT_REGION_PREEMPTION:
            if (ctx->args[0].value.u64) {
                ASSERT(_task == NO_TASK);
                _task = ctx->id;
            }
            break;

        default:
            break;
    }
}
REGISTER_HANDLER(SLOT_REGION_PREEMPTION, _region_preemption_handle)

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
STATIC void
_region_preemption_print(const marshable_t *m)
{
    logger_infof("nondefault tasks = ");
    tidset_print(m);
}

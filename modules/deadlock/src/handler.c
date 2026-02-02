#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/base/reason.h>
#include <lotto/base/tidbag.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/modules/deadlock/state.h>
#include <lotto/modules/mutex.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/casts.h>
#include <lotto/util/macros.h>
struct rsrc {
    mapitem_t ti;
    tidbag_t tasks;
    task_id owner;
};

static struct handler_deadlock {
    map_t resources;
    map_t lost;
} _state;


REGISTER_STATE(EPHEMERAL, _state, {
    map_init(&_state.resources, MARSHABLE_STATIC(sizeof(struct rsrc)));
    map_init(&_state.lost, MARSHABLE_STATIC(sizeof(struct rsrc)));
})

static struct rsrc *
_rsrc_init(map_t *m, uintptr_t addr)
{
    struct rsrc *rsrc = (struct rsrc *)map_find(m, addr);
    if (rsrc == NULL) {
        rsrc = (struct rsrc *)map_register(m, addr);
        ASSERT(rsrc);
        tidbag_init(&rsrc->tasks);
    }
    return rsrc;
}

static struct rsrc *
_rsrc_iterate(map_t *m)
{
    return (struct rsrc *)map_iterate(m);
}

static struct rsrc *
_rsrc_next(struct rsrc *it)
{
    return (struct rsrc *)map_next(&it->ti);
}

static void
_acquiring(task_id id, uintptr_t addr)
{
    logger_debugf("[%lx] aquiring resource 0x%lx\n", id, addr);
    struct rsrc *rsrc = _rsrc_init(&_state.resources, addr);
    tidbag_insert(&rsrc->tasks, id);
    rsrc->owner = mutex_owner((void *)addr);
}

static bool
_released(task_id id, uintptr_t addr)
{
    struct rsrc *rsrc = (struct rsrc *)map_find(&_state.resources, addr);
    if (!rsrc) {
        logger_errorf("an unacquired resource is being released\n");
        return !deadlock_config()->extra_release_check;
    }

    task_id owner = rsrc->owner;
    logger_debugf("[%lx] releasing resource 0x%lx owned by %lu\n", id, addr,
                  owner);
    tidbag_remove(&rsrc->tasks, id);

    if (tidbag_size(&rsrc->tasks) == 0) {
        tidbag_fini(&rsrc->tasks);
        map_deregister(&_state.resources, addr);
    } else {
        rsrc->owner = mutex_owner((void *)addr);
    }

    return true;
}

// call with cycle == NO_TASK
static bool
_check_deadlock_iter(task_id tid, uint64_t key, task_id cycle)
{
    if (tid == NO_TASK)
        return false;

    if (cycle == tid) {
        logger_errorf("Deadlock detected!\n");
        return true;
    }

    struct rsrc *rsrc = (struct rsrc *)map_find(&_state.resources, key);
    if (rsrc == NULL)
        return false;

    /* if no other task is in the resource, there is no deadlock */
    bool any_other = false;
    for (size_t idx = 0; idx < tidbag_size(&rsrc->tasks); idx++) {
        task_id cur = tidbag_get(&rsrc->tasks, idx);
        if (cur != tid) {
            any_other = true;
            break;
        }
    }
    if (!any_other)
        return false;

    /* for all tasks in resource */
    for (size_t idx = 0; idx < tidbag_size(&rsrc->tasks); idx++) {
        task_id cur = tidbag_get(&rsrc->tasks, idx);

        /* except myself */
        if (cur == tid)
            continue;

        /* find other resource where task is in */
        struct rsrc *it = _rsrc_iterate(&_state.resources);
        while (it) {
            if (!tidbag_has(&it->tasks, cur)) {
                it = _rsrc_next(it);
                continue;
            }

            tidbag_remove(&it->tasks, cur);

            /* task is in another resource, does this form a cycle? */
            bool res = _check_deadlock_iter(cur, it->ti.key,
                                            cycle == NO_TASK ? tid : cycle);

            tidbag_insert(&it->tasks, cur);

            if (res) {
                logger_errorf("  (tid: %lu) <- (rsrc: 0x%lx) <- (tid: %lu) \n",
                              cur, key, tid);
                return true;
            }

            it = _rsrc_next(it);
        }
    }
    return false;
}

static bool
_check_deadlock(task_id tid, uint64_t key)
{
    return _check_deadlock_iter(tid, key, NO_TASK);
}

static void
_lost_error_strict(task_id owner, uintptr_t addr)
{
    logger_errorf("Deadlock detected! (lost resource)\n");
    logger_errorf("Task %lu finished without releasing 0x%lx\n", owner, addr);
}

static void
_lost_error(task_id owner, task_id waiter, uintptr_t addr)
{
    _lost_error_strict(owner, addr);
    logger_errorf("Task %lu is waiting for 0x%lx\n", waiter, addr);
}

static bool
_is_lost(task_id id, uintptr_t addr)
{
    if (map_find(&_state.lost, addr) == NULL)
        return false;

    struct rsrc *it = _rsrc_iterate(&_state.lost);
    task_id owner   = tidbag_get(&it->tasks, 0);
    _lost_error(owner, id, addr);
    return true;
}

static bool
_mark_lost(task_id id)
{
    bool ret = false;

    for (struct rsrc *it = _rsrc_iterate(&_state.resources); it;
         it              = _rsrc_next(it)) {
        if (tidbag_has(&it->tasks, id)) {
            ret = true;

            uint64_t addr = it->ti.key;

            /* remove task from resource */
            if (tidbag_has(&it->tasks, id))
                tidbag_remove(&it->tasks, id);

            /* if some other task is waiting on this resource, it deadlocks.
             */
            if (tidbag_size(&it->tasks) > 0) {
                task_id waiter = tidbag_get(&it->tasks, 0);
                _lost_error(id, waiter, addr);
            } else {
                if (deadlock_config()->lost_resource_check) {
                    _lost_error_strict(id, addr);
                } else {
                    ret = false;
                }
            }

            /* find or create resource in lost map to warn future tasks
             * trying to acquire this resource */
            struct rsrc *rsrc = _rsrc_init(&_state.lost, addr);
            tidbag_insert(&rsrc->tasks, id);
        }
    }

    return ret;
}

STATIC void
_deadlock_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !deadlock_config()->enabled)
        return;

    ASSERT(ctx);

    /* This handler uses ctx->vid as task id if that field is properly set. In
     * case ctx->vid != NO_TASK, but the client cannot determine the proper
     * value, the client should set ctx->vid = ANY_TASK to force this handler
     * discard the event. If ctx->vid is not set (ie, it is NO_TASK), ctx->id is
     * used as task id. */
    if (ctx->vid == ANY_TASK)
        return;
    task_id tid = ctx->vid ? ctx->vid : ctx->id;

    ASSERT(tid != NO_TASK);
    uintptr_t addr = CAST_TYPE(uint64_t, ctx->args[0].value.ptr);
    switch (ctx->cat) {
        case CAT_MUTEX_ACQUIRE:
        case CAT_RSRC_ACQUIRING:
            if (_check_deadlock(tid, addr)) {
                e->reason = REASON_RSRC_DEADLOCK;
            }
            /* fallthru */
        case CAT_MUTEX_TRYACQUIRE:
            if (_is_lost(tid, addr)) {
                e->reason = REASON_RSRC_DEADLOCK;
            }
            if (ctx->args[1].value.u8 == 0) {
                _acquiring(tid, addr);
            }
            break;
        case CAT_MUTEX_RELEASE:
        case CAT_RSRC_RELEASED:
            if (!_released(tid, addr)) {
                e->reason = REASON_RSRC_DEADLOCK;
            }
            break;
        case CAT_TASK_FINI:
            if (_mark_lost(tid)) {
                e->reason = REASON_RSRC_DEADLOCK;
            }
            break;
        default:
            break;
    }
    if (e->reason == REASON_RSRC_DEADLOCK) {
        struct value val = bval(true);
        PS_PUBLISH_INTERFACE(TOPIC_DEADLOCK_DETECTED, val);
    }
}
REGISTER_HANDLER(SLOT_DEADLOCK, _deadlock_handle)

static tidset_t _dbg_set;

const tidset_t *
_lotto_deadlock_keys(void)
{
    tidset_fini(&_dbg_set);
    tidset_init(&_dbg_set);

    for (struct rsrc *it = _rsrc_iterate(&_state.resources); it;
         it              = _rsrc_next(it)) {
        tidset_insert(&_dbg_set, it->ti.key);
    }
    return &_dbg_set;
}

const tidset_t *
_lotto_deadlock_tasks_in(void *addr)
{
    struct rsrc *rsrc = (struct rsrc *)map_find(
        &_state.resources, CAST_TYPE(uint64_t, (uintptr_t)addr));
    if (rsrc == NULL)
        return NULL;

    tidset_fini(&_dbg_set);
    for (size_t idx = 0; idx < tidbag_size(&rsrc->tasks); idx++) {
        task_id cur = tidbag_get(&rsrc->tasks, idx);
        tidset_insert(&_dbg_set, cur);
    }
    return &_dbg_set;
}

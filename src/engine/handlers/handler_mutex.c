/*
 */
#include <errno.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK

#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/casts.h>
#include <lotto/util/macros.h>

struct mtx {
    mapitem_t ti;
    tidset_t waiters;
    task_id owner;
    int count;
};

static struct handler_mutex {
    map_t mutexes;
    tidset_t waiters;
} _state;


REGISTER_STATE(EPHEMERAL, _state, {
    map_init(&_state.mutexes, MARSHABLE_STATIC(sizeof(struct mtx)));
    tidset_init(&_state.waiters);
})

// call with cycle == NO_TASK
static bool
_check_deadlock(task_id waiter, uint64_t key, task_id cycle)
{
    ASSERT(waiter != NO_TASK);
    if (cycle == waiter) {
        logger_errorf("Deadlock detected!\n");
        logger_errorf("Wait chain: %lu <- ", waiter);
        return true;
    }

    struct mtx *mtx = (struct mtx *)map_find(&_state.mutexes, key);

    ASSERT(mtx->owner != NO_TASK);
    ASSERT(mtx->owner != waiter);

    /* if owner is not waiting for anybody, there is no deadlock */
    if (!tidset_has(&_state.waiters, mtx->owner))
        return false;

    const struct mtx *it = (struct mtx *)map_iterate(&_state.mutexes);
    for (; it && !tidset_has(&it->waiters, mtx->owner);
         it = (const struct mtx *)map_next(&it->ti)) {}
    ASSERT(it != NULL);

    if (_check_deadlock(mtx->owner, it->ti.key,
                        cycle == NO_TASK ? waiter : cycle)) {
        logger_printf("%lu", waiter);
        logger_printf("%s", (cycle == NO_TASK ? "\n" : " <- "));
        return true;
    }
    return false;
}

static struct mtx *
_mutex_init(uint64_t addr)
{
    struct mtx *mtx = (struct mtx *)map_find(&_state.mutexes, addr);
    if (mtx == NULL) {
        mtx = (struct mtx *)map_register(&_state.mutexes, addr);
        ASSERT(mtx);
        tidset_init(&mtx->waiters);
        mtx->owner = NO_TASK;
        mtx->count = 0;
    }
    return mtx;
}

static bool
_remove_waiters(tidset_t *tset, task_id self)
{
    bool should_wait = false;
    for (const struct mtx *it = (struct mtx *)map_iterate(&_state.mutexes);
         it != NULL; it       = (const struct mtx *)map_next(&it->ti)) {
        if (it->owner == NO_TASK) {
            continue;
        }
        tidset_subtract(tset, &it->waiters);
        should_wait |= tidset_has(&it->waiters, self);
    }
    return should_wait;
}

static bool
_should_wait(task_id id)
{
    for (const struct mtx *it = (struct mtx *)map_iterate(&_state.mutexes);
         it != NULL; it       = (const struct mtx *)map_next(&it->ti)) {
        if (it->owner == NO_TASK) {
            continue;
        }
        if (tidset_has(&it->waiters, id)) {
            return true;
        }
    }
    return false;
}

static int
_posthandle_tryacquire(task_id id, uint64_t addr)
{
    logger_debugf("[%lu] mutex tryaquire 0x%lx\n", id, addr);
    ASSERT(!tidset_has(&_state.waiters, id));

    struct mtx *mtx = _mutex_init(addr);

    if (mtx->owner == NO_TASK) {
        ASSERT(mtx->count == 0);
        mtx->owner = id;
    }

    if (mtx->owner == id) {
        mtx->count++;
        logger_debugf("[%lu] mutex (try)aquired 0x%lx (count: %d)\n", id, addr,
                   mtx->count);
        return 0;
    }
    return EBUSY;
}

static void
_handle_acquire(task_id id, uint64_t addr)
{
    logger_debugf("[%lu] mutex aquire 0x%lx\n", id, addr);
    ASSERT(!tidset_has(&_state.waiters, id));

    struct mtx *mtx = _mutex_init(addr);

    if (mtx->owner == id) {
        return;
    }
    /* a task can only be waiting for a single mutex at a time */
    ENSURE(tidset_insert(&mtx->waiters, id));
}

static void
_posthandle_acquire(task_id id, uint64_t addr)
{
    logger_debugf("[%lu] mutex aquire 0x%lx\n", id, addr);
    ASSERT(!tidset_has(&_state.waiters, id));

    struct mtx *mtx = _mutex_init(addr);

    if (mtx->owner == NO_TASK) {
        ASSERT(mtx->count == 0);
        mtx->owner = id;
        ENSURE(tidset_remove(&mtx->waiters, id));
    }

    ASSERT(mtx->owner == id && "deadlock due to disrespecting locks");
    mtx->count++;
    logger_debugf("[%lu] mutex aquired 0x%lx (count: %d)\n", id, addr, mtx->count);
}

static void
_posthandle_release(task_id id, uint64_t addr)
{
    struct mtx *mtx = (struct mtx *)map_find(&_state.mutexes, addr);
    if (!mtx) {
        logger_errorf("an unacquired mutex is being released\n");
        return;
    }

    logger_debugf("[%lu] mutex release 0x%lx (count: %d)\n", id, addr,
               mtx->count - 1);

    if (mtx->owner != id) {
        logger_errorf(
            "undefined behavior: task %lu releases mutex 0x%lx owned by task "
            "%lu\n",
            id, addr, mtx->owner);
    }
    ASSERT(mtx->count > 0);
    if (--mtx->count > 0)
        return;
    mtx->owner = NO_TASK;
    ASSERT(mtx->count == 0);

    if (tidset_size(&mtx->waiters) == 0) {
        tidset_fini(&mtx->waiters);
        map_deregister(&_state.mutexes, addr);
    }
}

STATIC void
_mutex_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !mutex_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    uint64_t addr = CAST_TYPE(uint64_t, ctx->args[0].value.ptr);
    switch (ctx->cat) {
        case CAT_MUTEX_ACQUIRE:
            _handle_acquire(ctx->id, addr);
            cappt_add_any_task_filter(e, _should_wait);
            // fallthru
        case CAT_MUTEX_TRYACQUIRE:
        case CAT_MUTEX_RELEASE:
            e->is_chpt = true;
            break;
        default:
            break;
    }

    /* remove waiting tasks from the tset */
    if (_remove_waiters(&e->tset, ctx->id) && mutex_config()->deadlock_check &&
        _check_deadlock(ctx->id, addr, NO_TASK)) {
        logger_errorf("Aborting on deadlock\n");
        e->reason = REASON_RSRC_DEADLOCK;
    }
}
REGISTER_HANDLER(SLOT_MUTEX, _mutex_handle)

PS_SUBSCRIBE_INTERFACE(TOPIC_NEXT_TASK, {
    const context_t *ctx = (context_t *)as_any(v);
    ASSERT(ctx);

    uint64_t addr = (uint64_t)ctx->args[0].value.ptr;
    switch (ctx->cat) {
        case CAT_MUTEX_ACQUIRE:
            _posthandle_acquire(ctx->id, addr);
            break;
        case CAT_MUTEX_TRYACQUIRE: {
            arg_t *ok    = (arg_t *)&ctx->args[1];
            ok->value.u8 = _posthandle_tryacquire(ctx->id, addr);
        } break;
        case CAT_MUTEX_RELEASE:
            _posthandle_release(ctx->id, addr);
            break;
        default:
            break;
    }
})

struct handler_mutex *
_lotto_mutex_handler()
{
    return &_state;
}

task_id
_lotto_mutex_owner(void *addr)
{
    struct mtx *mtx = (struct mtx *)map_find(
        &_state.mutexes, CAST_TYPE(uint64_t, (uintptr_t)addr));
    if (mtx == NULL)
        return NO_TASK;
    return mtx->owner;
}

void
_lotto_print_mutex_waiters(void)
{
    tidset_print(&_state.waiters.m);
}

bool
_lotto_mutex_is_waiter_of(task_id id, void *addr)
{
    struct mtx *mtx = (struct mtx *)map_find(
        &_state.mutexes, CAST_TYPE(uint64_t, (uintptr_t)addr));
    if (mtx == NULL)
        return false;
    return tidset_has(&mtx->waiters, id);
}

bool
_lotto_mutex_is_waiting(task_id id)
{
    return tidset_has(&_state.waiters, id);
}

task_id
mutex_owner(void *addr)
{
    struct mtx *mtx = (struct mtx *)map_find(&_state.mutexes, (uint64_t)addr);
    return mtx == NULL ? NO_TASK : mtx->owner;
}

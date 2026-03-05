#include <errno.h>
#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/brokers/pubsub.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/brokers/statemgr.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/modules/rwlock/state.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger_block.h>
#include <lotto/util/casts.h>
#include <lotto/util/macros.h>

struct reader {
    mapitem_t ti;
    int cnt; // number of recursive read locks
};

struct rwlock {
    mapitem_t ti;
    task_id  writer;
    tidmap_t readers;
    tidset_t read_waiters;
    tidset_t write_waiters;
};

static struct handler_rwlock {
    map_t locks;
} _state;

REGISTER_STATE(EPHEMERAL, _state, {
    map_init(&_state.locks, MARSHABLE_STATIC(sizeof(struct rwlock)));
})

static struct rwlock * _rwlock_init(uint64_t addr);
STATIC bool _rwlock_is_read_locked(const struct rwlock *lock);
STATIC bool _rwlock_is_read_locked_by(const struct rwlock *lock, task_id id);
STATIC bool _rwlock_is_write_locked(const struct rwlock *lock);
STATIC bool _handle_rdlock(task_id id, uint64_t addr, event_t *e);
STATIC bool _handle_wrlock(task_id id, uint64_t addr, event_t *e);
STATIC void _posthandle_rdlock(task_id id, uintptr_t addr);
STATIC int _posthandle_tryrdlock(task_id id, uintptr_t addr);
STATIC void _posthandle_wrlock(task_id id, uintptr_t addr);
STATIC int _posthandle_trywrlock(task_id id, uintptr_t addr);
STATIC void _posthandle_unlock(task_id id, uintptr_t addr);
STATIC bool _should_wait(task_id id);

STATIC void
_rwlock_handle(const context_t *ctx, event_t *e)
{
    ASSERT(e);
    if (e->skip || !rwlock_config()->enabled)
        return;

    ASSERT(ctx);
    ASSERT(ctx->id != NO_TASK);
    uint64_t addr = CAST_TYPE(uint64_t, ctx->args[0].value.ptr);
    switch (ctx->cat) {
        case CAT_RWLOCK_RDLOCK:
            _handle_rdlock(ctx->id, addr, e);
            e->is_chpt = true;
            ASSERT(!e->any_task_filter);
            e->any_task_filter = _should_wait;
            break;
        
        case CAT_RWLOCK_WRLOCK:
            _handle_wrlock(ctx->id, addr, e);
            e->is_chpt = true;
            ASSERT(!e->any_task_filter);
            e->any_task_filter = _should_wait;
            break;

        case CAT_RWLOCK_TRYRDLOCK:
        case CAT_RWLOCK_TRYWRLOCK:
        case CAT_RWLOCK_UNLOCK:
            e->is_chpt = true;
            break;

        default:
            break;
    }

    const struct rwlock *it = (struct rwlock *)map_iterate(&_state.locks);
    for (; it; it = (const struct rwlock *)map_next(&it->ti)) {
        if (_rwlock_is_read_locked(it)) {
            tidset_subtract(&e->tset, &it->write_waiters);
        } else if (_rwlock_is_write_locked(it)) {
            tidset_subtract(&e->tset, &it->read_waiters);
            tidset_subtract(&e->tset, &it->write_waiters);
        }
    }
}
REGISTER_HANDLER(SLOT_RWLOCK, _rwlock_handle)

PS_SUBSCRIBE_INTERFACE(TOPIC_NEXT_TASK, {
    context_t *ctx = (context_t *)as_any(v);
    ASSERT(ctx);

    uint64_t addr = (uint64_t)ctx->args[0].value.ptr;
    switch (ctx->cat) {
        case CAT_RWLOCK_RDLOCK:
            _posthandle_rdlock(ctx->id, addr);
            break;
        case CAT_RWLOCK_WRLOCK:
            _posthandle_wrlock(ctx->id, addr);
            break;
        case CAT_RWLOCK_UNLOCK:
            _posthandle_unlock(ctx->id, addr);
            break;
        case CAT_RWLOCK_TRYRDLOCK:
            ctx->args[1].value.u32 = (uint32_t)_posthandle_tryrdlock(ctx->id, addr);
            break;
        case CAT_RWLOCK_TRYWRLOCK:
            ctx->args[1].value.u32 = (uint32_t)_posthandle_trywrlock(ctx->id, addr);
            break;
        default:
            break;
    }
})

//
// rwlock
//
static struct rwlock *
_rwlock_init(uint64_t addr)
{
    struct rwlock *lock = (struct rwlock *)map_find(&_state.locks, addr);
    if (!lock) {
        lock = (struct rwlock *)map_register(&_state.locks, addr);
        ASSERT(lock);
        lock->writer = NO_TASK;
        map_init(&lock->readers, MARSHABLE_STATIC(sizeof(struct reader)));
        tidset_init(&lock->read_waiters);
        tidset_init(&lock->write_waiters);
    }
    return lock;
}

STATIC inline bool
_rwlock_is_read_locked(const struct rwlock *lock)
{
    size_t n = tidmap_size(&lock->readers);
    ASSERT(n == 0 || lock->writer == NO_TASK);
    return n > 0;
}

STATIC inline bool
_rwlock_is_read_locked_by(const struct rwlock *lock, task_id id)
{
    return tidmap_find(&lock->readers, id) != NULL;
}

STATIC inline bool
_rwlock_is_write_locked(const struct rwlock *lock)
{
    return lock->writer != NO_TASK;
}

STATIC bool
_should_wait(task_id id)
{
    const struct rwlock *it = (struct rwlock *)map_iterate(&_state.locks);
    for (; it; it = (const struct rwlock *)map_next(&it->ti)) {
        if (_rwlock_is_write_locked(it)) {
            if (tidset_has(&it->write_waiters, id) || tidset_has(&it->read_waiters, id)) {
                return true;
            }
        } else if (_rwlock_is_read_locked(it)) {
            if (tidset_has(&it->write_waiters, id)) {
                return true;
            }
        }
    }
    return false;
}

//
// handle
//
STATIC bool
_handle_rdlock(task_id id, uint64_t addr, event_t *e)
{
    logger_debugf("[%lu] rwlock rdlock 0x%lx\n", id, addr);
    struct rwlock *lock = _rwlock_init(addr);
    if (_rwlock_is_write_locked(lock)) {
        if (lock->writer == id) {
            e->reason = REASON_RSRC_DEADLOCK;
            return false;
        }
    }
    ENSURE(tidset_insert(&lock->read_waiters, id));
    return true;
}

STATIC bool
_handle_wrlock(task_id id, uint64_t addr, event_t *e)
{
    logger_debugf("[%lu] rwlock wrlock 0x%lx\n", id, addr);
    struct rwlock *lock = _rwlock_init(addr);
    bool is_write_locked = _rwlock_is_write_locked(lock);
    bool is_read_locked = _rwlock_is_read_locked(lock);
    if (is_write_locked && lock->writer == id || is_read_locked && _rwlock_is_read_locked_by(lock, id)) {
        e->reason = REASON_RSRC_DEADLOCK;
        return false;
    }
    ENSURE(tidset_insert(&lock->write_waiters, id));
    return true;
}

//
// posthandle
//
STATIC void
_posthandle_rdlock(task_id id, uintptr_t addr)
{
    struct rwlock *lock = _rwlock_init(addr);
    ASSERT(!_rwlock_is_write_locked(lock));
    ENSURE(tidset_remove(&lock->read_waiters, id));
    struct reader *reader = (struct reader *)tidmap_find(&lock->readers, id);
    if (!reader) {
        reader = (struct reader *)tidmap_register(&lock->readers, id);
        reader->cnt = 0;
    }
    reader->cnt++;
    logger_debugf("rwlock 0x%lx is read locked by %lu (cnt=%d)\n", addr, id, reader->cnt);
}

STATIC int
_posthandle_tryrdlock(task_id id, uintptr_t addr)
{
    struct rwlock *lock = _rwlock_init(addr);
    if (_rwlock_is_write_locked(lock)) {
        if (lock->writer == id) {
            return EDEADLK;
        }
        return EBUSY;
    } else {
        struct reader *reader = (struct reader *)tidmap_find(&lock->readers, id);
        if (!reader) {
            reader = (struct reader *)tidmap_register(&lock->readers, id);
            reader->cnt = 0;
        }
        reader->cnt++;
        logger_debugf("rwlock 0x%lx is (try)read locked by %lu (cnt=%d)\n", addr, id, reader->cnt);
        return 0;
    }
}

STATIC void
_posthandle_wrlock(task_id id, uintptr_t addr)
{
    struct rwlock *lock = _rwlock_init(addr);
    ASSERT(!_rwlock_is_read_locked(lock));
    ASSERT(lock->writer == NO_TASK);
    ENSURE(tidset_remove(&lock->write_waiters, id));
    lock->writer = id;
    logger_debugf("rwlock 0x%lx is write locked by %lu\n", addr, id);
}

STATIC int
_posthandle_trywrlock(task_id id, uintptr_t addr)
{
    struct rwlock *lock = _rwlock_init(addr);
    if (lock->writer == id) 
        return EDEADLK;
    if (_rwlock_is_read_locked(lock) || _rwlock_is_write_locked(lock)) {
        return EBUSY;
    }
    lock->writer = id;
    logger_debugf("rwlock 0x%lx is (try)write locked by %lu\n", addr, id);
    return 0;
}

STATIC void
_posthandle_unlock(task_id id, uintptr_t addr)
{
    struct rwlock *lock = _rwlock_init(addr);
    if (lock->writer != NO_TASK) {
        ASSERT(lock->writer == id);
        lock->writer = NO_TASK;
        logger_debugf("rwlock 0x%lx is unlocked by writer %lu\n", addr, id);
    } else {
        struct reader *reader = (struct reader *)tidmap_find(&lock->readers, id);
        ASSERT(reader && reader->cnt >= 0);
        reader->cnt--;
        logger_debugf("rwlock 0x%lx is unlocked by reader %lu (count=%d)\n", addr, id, reader->cnt);
        if (reader->cnt == 0) {
            tidmap_deregister(&lock->readers, id);
        }
    }
}

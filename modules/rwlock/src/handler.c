#include <errno.h>

#include "state.h"
#include <dice/events/pthread.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/rwlock/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

struct reader {
    mapitem_t ti;
    int cnt; // number of recursive read locks
};

struct rwlock {
    mapitem_t ti;
    task_id writer;
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

static struct rwlock *_rwlock_init(uint64_t addr);
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
STATIC type_id _rwlock_event(const capture_point *cp);
STATIC uint64_t _rwlock_addr(const capture_point *cp);
STATIC void _rwlock_try_set_ret(const capture_point *cp, int ret);

STATIC void
_rwlock_handle(const capture_point *cp, event_t *e)
{
    ASSERT(e);
    if (e->skip || !rwlock_config()->enabled)
        return;

    ASSERT(cp);
    ASSERT(cp->id != NO_TASK);
    switch (_rwlock_event(cp)) {
        case EVENT_RWLOCK_RDLOCK:
        case EVENT_RWLOCK_TIMEDRDLOCK:
            _handle_rdlock(cp->id, _rwlock_addr(cp), e);
            e->is_chpt = true;
            ASSERT(!e->any_task_filter);
            e->any_task_filter = _should_wait;
            break;

        case EVENT_RWLOCK_WRLOCK:
        case EVENT_RWLOCK_TIMEDWRLOCK:
            _handle_wrlock(cp->id, _rwlock_addr(cp), e);
            e->is_chpt = true;
            ASSERT(!e->any_task_filter);
            e->any_task_filter = _should_wait;
            break;

        case EVENT_RWLOCK_TRYRDLOCK:
        case EVENT_RWLOCK_TRYWRLOCK:
        case EVENT_RWLOCK_UNLOCK:
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
        } else if (tidset_size(&it->write_waiters)) {
            tidset_subtract(&e->tset, &it->read_waiters);
        }
    }
}
ON_SEQUENCER_CAPTURE(_rwlock_handle)

LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {
    const capture_point *cp = EVENT_PAYLOAD(cp);
    ASSERT(cp);

    switch (_rwlock_event(cp)) {
        case EVENT_RWLOCK_RDLOCK:
        case EVENT_RWLOCK_TIMEDRDLOCK:
            _posthandle_rdlock(cp->id, _rwlock_addr(cp));
            break;
        case EVENT_RWLOCK_WRLOCK:
        case EVENT_RWLOCK_TIMEDWRLOCK:
            _posthandle_wrlock(cp->id, _rwlock_addr(cp));
            break;
        case EVENT_RWLOCK_UNLOCK:
            _posthandle_unlock(cp->id, _rwlock_addr(cp));
            break;
        case EVENT_RWLOCK_TRYRDLOCK:
            _rwlock_try_set_ret(
                cp, _posthandle_tryrdlock(cp->id, _rwlock_addr(cp)));
            break;
        case EVENT_RWLOCK_TRYWRLOCK:
            _rwlock_try_set_ret(
                cp, _posthandle_trywrlock(cp->id, _rwlock_addr(cp)));
            break;
        default:
            break;
    }
})

STATIC type_id
_rwlock_event(const capture_point *cp)
{
    return cp->src_type;
}

STATIC uint64_t
_rwlock_addr(const capture_point *cp)
{
    switch (_rwlock_event(cp)) {
        case EVENT_RWLOCK_RDLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(struct rwlock_rdlock_event *)
                ->lock;
        case EVENT_RWLOCK_WRLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(struct rwlock_wrlock_event *)
                ->lock;
        case EVENT_RWLOCK_UNLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(struct rwlock_unlock_event *)
                ->lock;
        case EVENT_RWLOCK_TRYRDLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(
                       struct rwlock_tryrdlock_event *)
                ->lock;
        case EVENT_RWLOCK_TRYWRLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(
                       struct rwlock_trywrlock_event *)
                ->lock;
        case EVENT_RWLOCK_TIMEDRDLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(
                       struct rwlock_timedrdlock_event *)
                ->lock;
        case EVENT_RWLOCK_TIMEDWRLOCK:
            return (uint64_t)(uintptr_t)CP_PAYLOAD(
                       struct rwlock_timedwrlock_event *)
                ->lock;
        default:
            ASSERT(0);
            return 0;
    }
}

STATIC void
_rwlock_try_set_ret(const capture_point *cp, int ret)
{
    switch (_rwlock_event(cp)) {
        case EVENT_RWLOCK_TRYRDLOCK:
            CP_PAYLOAD(struct rwlock_tryrdlock_event *)->ret = ret;
            return;
        case EVENT_RWLOCK_TRYWRLOCK:
            CP_PAYLOAD(struct rwlock_trywrlock_event *)->ret = ret;
            return;
        default:
            ASSERT(0);
            return;
    }
}

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
            if (tidset_has(&it->write_waiters, id) ||
                tidset_has(&it->read_waiters, id)) {
                return true;
            }
        } else if (_rwlock_is_read_locked(it)) {
            if (tidset_has(&it->write_waiters, id)) {
                return true;
            }
        } else if (tidset_size(&it->write_waiters) > 0) {
            if (tidset_has(&it->read_waiters, id)) {
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
    struct rwlock *lock  = _rwlock_init(addr);
    bool is_write_locked = _rwlock_is_write_locked(lock);
    bool is_read_locked  = _rwlock_is_read_locked(lock);
    if (is_write_locked && lock->writer == id ||
        is_read_locked && _rwlock_is_read_locked_by(lock, id)) {
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
        reader      = (struct reader *)tidmap_register(&lock->readers, id);
        reader->cnt = 0;
    }
    reader->cnt++;
    logger_debugf("rwlock 0x%lx is read locked by %lu (cnt=%d)\n", addr, id,
                  reader->cnt);
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
    }
    if (tidset_size(&lock->write_waiters) > 0) {
        return EBUSY;
    }
    struct reader *reader = (struct reader *)tidmap_find(&lock->readers, id);
    if (!reader) {
        reader      = (struct reader *)tidmap_register(&lock->readers, id);
        reader->cnt = 0;
    }
    reader->cnt++;
    logger_debugf("rwlock 0x%lx is (try)read locked by %lu (cnt=%d)\n", addr,
                  id, reader->cnt);
    return 0;
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
    } else if (_rwlock_is_read_locked_by(lock, id)) {
        struct reader *reader =
            (struct reader *)tidmap_find(&lock->readers, id);
        ASSERT(reader && reader->cnt >= 0);
        reader->cnt--;
        logger_debugf("rwlock 0x%lx is unlocked by reader %lu (count=%d)\n",
                      addr, id, reader->cnt);
        if (reader->cnt == 0) {
            tidmap_deregister(&lock->readers, id);
        }
    } else {
        logger_warnf(
            "undefined behavior: task %lu tries to unlock an unacquired rwlock "
            "0x%lx\n",
            id, addr);
    }
}

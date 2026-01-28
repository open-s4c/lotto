/*
 */
/*******************************************************************************
 * Switcher is a sort of barrier where tasks get blocked until it's their
 * turn. The interface is basically yield() to block a task and wake() to
 * enable a specific task. The task enabled does not necessarily has to be
 * in the switcher at the moment it becomes enabled (it may arrive a few
 * instants later).
 *
 * Note that this implementation is simplistic. We know it is very inefficiently
 * using the condition variable and signals until the next task gets
 * scheduled. An ideal implementation would wake up (from the futex) only the
 * task that is being awaken (from the switcher). A possible implementation
 * would be to bucketize the futexes and let tasks select the bucket based on
 * the hash of the ids.
 ******************************************************************************/
#include <stdbool.h>
#include <stdio.h>
// clang-format off
#include <vsync/thread/mutex.h>
#include <vsync/thread/cond.h>
// clang-format on

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/cappt.h>
#include <lotto/base/task_id.h>
#include <lotto/runtime/switcher.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>

#ifndef LOTTO_SWITCHER_NBUCKETS
    #define LOTTO_SWITCHER_NBUCKETS 128
#endif

typedef struct {
    vmutex_t mutex;
    vcond_t cnd[LOTTO_SWITCHER_NBUCKETS];
    int cnd_counter[LOTTO_SWITCHER_NBUCKETS];
    task_id next;
    task_id prev;
    switcher_status_t status;
    nanosec_t slack;
} switcher_t;

static switcher_t _switcher;

void
_lotto_switcher_resuming(void)
{
}

static inline bool
may_wake_as_any_task(task_id id, any_task_filters filters)
{
    for (int i = 0; i < filters.n; ++i) {
        any_task_filter_f f = filters.val[i];
        if (f && !f(id))
            return false;
    }
    return true;
}

switcher_status_t
switcher_yield(task_id id, any_task_filters filters)
{
    task_id prev, next;
    logger_debugf("YIELD  task %lu\n", id);

    vmutex_acquire(&_switcher.mutex);
    int bucket = (int)(id % LOTTO_SWITCHER_NBUCKETS);

    /* before the slack time passes, we still have a chance to continue */
    if (_switcher.slack && _switcher.prev == id) {
        next = id;
    }

    _switcher.cnd_counter[bucket]++;

    next = _switcher.next;
    while (next != id && _switcher.status != SWITCHER_ABORTED &&
           (next != ANY_TASK || may_wake_as_any_task(id, filters))) {
        vcond_signal(&_switcher.cnd[bucket]);
        vcond_wait(&_switcher.cnd[bucket], &_switcher.mutex);
        if (_switcher.slack) {
            struct timespec ts = to_timespec(_switcher.slack);
            vmutex_release(&_switcher.mutex);
            nanosleep(&ts, NULL);
            vmutex_acquire(&_switcher.mutex);
            _switcher.slack = 0;
        }
        next = _switcher.next;
    }
    _switcher.next = NO_TASK;

    _switcher.cnd_counter[bucket]--;

    if (_switcher.status == SWITCHER_ABORTED) {
        vmutex_release(&_switcher.mutex);
        return SWITCHER_ABORTED;
    }

    prev            = _switcher.prev;
    _switcher.prev  = id;
    _switcher.slack = 0;

    switcher_status_t status;

    _switcher.status =
        prev != id || next == ANY_TASK ? SWITCHER_CHANGED : SWITCHER_CONTINUE;

    status = _switcher.status;

    vmutex_release(&_switcher.mutex);

    _lotto_switcher_resuming();
    logger_debugf("RESUME task %lu\n", id);

    return status;
}

vatomic32_t check;

void
switcher_wake(task_id id, nanosec_t slack)
{
    vmutex_acquire(&_switcher.mutex);
    logger_debugf("WAKE   task %lu\n", id);

    ASSERT(_switcher.next == NO_TASK);
    ASSERT(id != NO_TASK);

    int bucket = (int)id % LOTTO_SWITCHER_NBUCKETS;

    _switcher.next  = id;
    _switcher.slack = slack;
    if (id == ANY_TASK) {
        for (int i = 0; i < LOTTO_SWITCHER_NBUCKETS; i++) {
            if (_switcher.cnd_counter[i] > 0) {
                vcond_signal(&_switcher.cnd[i]);
            }
        }
    } else {
        vcond_signal(&_switcher.cnd[bucket]);
    }
    vmutex_release(&_switcher.mutex);
}

void
switcher_abort()
{
    vmutex_acquire(&_switcher.mutex);
    logger_debugf("ABORT called\n");
    _switcher.status = SWITCHER_ABORTED;
    vmutex_release(&_switcher.mutex);
}

/*
 */
#ifndef LOTTO_RSRC_DEADLOCK_H
#define LOTTO_RSRC_DEADLOCK_H
/**
 * @file deadlock.h
 * @brief Deadlock detector
 *
 * Resources are any object that requires exclusive access and can be identified
 * by a pointer.
 *
 */

#include <stdbool.h>
#include <stddef.h>

void _lotto_rsrc_acquiring(void *addr) __attribute__((weak));
void _lotto_rsrc_released(void *addr) __attribute__((weak));

/**
 * Informs that the current task is acquiring the resource.
 *
 * This marker is placed **before** the task actually acquires the resouce,
 * for example, a just before `mutex_lock()`. The actual resource aquisition
 * might block. For non-blocking resource acquisition, for example,
 * `mutex_trylock()`, refer to `lotto_rsrc_tried_acquiring`.
 *
 * @param addr opaque resource identifier
 */
static inline void
lotto_rsrc_acquiring(void *addr)
{
    if (_lotto_rsrc_acquiring != NULL) {
        _lotto_rsrc_acquiring(addr);
    }
}

/**
 * Informs that the current task has tried to acquire the resource.
 *
 * This marker is placed **after** the task tried to acquire a resource,
 * for example, after `mutex_trylock()`. Even though a trylock cannot cause
 * a hang, the algorithm needs to record the information that the resource
 * was actually acquired to be able to detect deadlocks in other conditions.
 *
 * @param addr opaque resource identifier
 * @param success whether task succeeded acquiring or not
 * @return value of success parameter
 */
static inline bool
lotto_rsrc_tried_acquiring(void *addr, bool success)
{
    if (success) {
        lotto_rsrc_acquiring(addr);
    }
    return success;
}

/**
 * Informs that the current task has released the resource.
 *
 * This marker should be placed **after** the resource has been released.
 *
 * @param addr opaque resource identifier
 */
static inline void
lotto_rsrc_released(void *addr)
{
    if (_lotto_rsrc_released != NULL) {
        _lotto_rsrc_released(addr);
    }
}

#endif

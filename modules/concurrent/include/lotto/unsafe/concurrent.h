/**
 * @file concurrent.h
 * @brief The concurrent interface.
 *
 * The concurrent interface allows the user to relax Lotto's enforced
 * sequential execution in regions that are known to work on purely local data.
 * The current task detaches from Lotto scheduling while such a region runs.
 *
 * Lotto observes the outermost enter/leave as a blocking BEFORE/AFTER pair, so
 * code inside the region can execute concurrently with the rest of the program
 * while regular capture points from that task are ignored.
 *
 * The exceptions are task lifecycle events: EVENT_TASK_CREATE,
 * EVENT_TASK_JOIN, and EVENT_TASK_FINI are still handled normally, while
 * EVENT_TASK_INIT during a concurrent region is invalid and aborts. This is an
 * unsafe interface and the user is not recommended to use it.
 */
#ifndef LOTTO_UNSAFE_CONCURRENT_H
#define LOTTO_UNSAFE_CONCURRENT_H

#include <stdbool.h>

#define lotto_concurrent(...)                                                  \
    {                                                                          \
        char concurrent                                                        \
            __attribute__((__cleanup__(lotto_concurrent_cleanup)));            \
        (void)concurrent;                                                      \
        lotto_concurrent_enter();                                              \
        __VA_ARGS__;                                                           \
    }

void _lotto_concurrent_enter(void) __attribute__((weak));
void _lotto_concurrent_leave(void) __attribute__((weak));
bool _lotto_is_concurrent(void) __attribute__((weak));

/**
 * Enter a concurrent region explicitly.
 */
static inline void
lotto_concurrent_enter(void)
{
    if (_lotto_concurrent_enter != NULL) {
        _lotto_concurrent_enter();
    }
}

/**
 * Leave a concurrent region explicitly.
 */
static inline void
lotto_concurrent_leave(void)
{
    if (_lotto_concurrent_leave != NULL) {
        _lotto_concurrent_leave();
    }
}

/**
 * Returns whether the current task is inside a concurrent region.
 */
static inline bool
lotto_is_concurrent(void)
{
    return _lotto_is_concurrent != NULL && _lotto_is_concurrent();
}

static inline void __attribute__((unused))
lotto_concurrent_cleanup(void *concurrent)
{
    (void)concurrent;
    lotto_concurrent_leave();
}

#endif

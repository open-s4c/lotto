/**
 * @file ghost.h
 * @brief The interceptor ghost interface.
 *
 * The ghost interface allows the current task to continue executing without
 * giving up the Lotto "core" to another Lotto task. While inside a ghost
 * region, Lotto ignores capture points from that task and execution continues
 * through the block.
 *
 * The exceptions are task lifecycle events: EVENT_TASK_CREATE,
 * EVENT_TASK_JOIN, and EVENT_TASK_FINI are still handled normally, while
 * EVENT_TASK_INIT during a ghost region is invalid and aborts. This is an
 * unsafe interface and the user is not recommended to use it.
 */
#ifndef LOTTO_UNSAFE_GHOST_H
#define LOTTO_UNSAFE_GHOST_H

#include <stdbool.h>
#include <stddef.h>

#define lotto_ghost(...)                                                       \
    {                                                                          \
        char ghost __attribute__((__cleanup__(lotto_ghost_cleanup)));          \
        (void)ghost;                                                           \
        lotto_ghost_enter();                                                   \
        __VA_ARGS__;                                                           \
    }

void _lotto_ghost_enter(void) __attribute__((weak));
void _lotto_ghost_leave(void) __attribute__((weak));
bool _lotto_ghosted(void) __attribute__((weak));

/**
 * Enter a ghost region.
 */
static inline void
lotto_ghost_enter(void)
{
    if (_lotto_ghost_enter != NULL) {
        _lotto_ghost_enter();
    }
}

/**
 * Leave a ghost region.
 */
static inline void
lotto_ghost_leave(void)
{
    if (_lotto_ghost_leave != NULL) {
        _lotto_ghost_leave();
    }
}

/**
 * Returns whether the interceptor is ghosted.
 */
static inline bool
lotto_ghosted(void)
{
    return _lotto_ghosted != NULL && _lotto_ghosted();
}

static inline void __attribute__((unused))
lotto_ghost_cleanup(void *ghost)
{
    (void)ghost;
    lotto_ghost_leave();
}

#endif

/*
 */
#ifndef LOTTO_REGION_ATOMIC_H
#define LOTTO_REGION_ATOMIC_H
/**
 * @file region_atomic.h
 * @brief The atomic region interface.
 *
 * The (non)atomic region enables/disables task preemptions. The default state
 * (outside the region) is controlled by the CLI flag
 * `--preemptions-off`. If the flag is set, preemptions are disabled and
 * nonatomic regions make a difference by allowing them. Otherwise, atomic
 * regions can be used to disable preemptions locally.
 */

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t task_id;

/**
 * Define an atomic region.
 */
#define lotto_atomic(...)                                                      \
    {                                                                          \
        char ghost __attribute__((__cleanup__(lotto_region_atomic_cleanup)));  \
        (void)ghost;                                                           \
        lotto_region_atomic_enter();                                           \
        __VA_ARGS__;                                                           \
    }

/**
 * Define a nonatomic region.
 */
#define lotto_nonatomic(...)                                                   \
    {                                                                          \
        char ghost                                                             \
            __attribute__((__cleanup__(lotto_region_nonatomic_cleanup)));      \
        (void)ghost;                                                           \
        lotto_region_nonatomic_enter();                                        \
        __VA_ARGS__;                                                           \
    }

void _lotto_region_atomic_enter(void) __attribute__((weak));
void _lotto_region_atomic_leave(void) __attribute__((weak));
void _lotto_region_atomic_enter_cond(bool cond) __attribute__((weak));
void _lotto_region_atomic_leave_cond(bool cond) __attribute__((weak));
void _lotto_region_atomic_enter_task(task_id task) __attribute__((weak));
void _lotto_region_atomic_leave_task(task_id task) __attribute__((weak));
void _lotto_region_nonatomic_enter(void) __attribute__((weak));
void _lotto_region_nonatomic_leave(void) __attribute__((weak));
void _lotto_region_nonatomic_enter_cond(bool cond) __attribute__((weak));
void _lotto_region_nonatomic_leave_cond(bool cond) __attribute__((weak));
void _lotto_region_nonatomic_enter_task(task_id task) __attribute__((weak));
void _lotto_region_nonatomic_leave_task(task_id task) __attribute__((weak));

/**
 * Enter the atomic region.
 */
static inline void
lotto_region_atomic_enter(void)
{
    if (_lotto_region_atomic_enter != NULL) {
        _lotto_region_atomic_enter();
    }
}
/**
 * Leave the atomic region.
 */
static inline void
lotto_region_atomic_leave(void)
{
    if (_lotto_region_atomic_leave != NULL) {
        _lotto_region_atomic_leave();
    }
}
/**
 * Enter the atomic region if the condition is met.
 *
 * @param cond condition
 */
static inline void
lotto_region_atomic_enter_cond(bool cond)
{
    if (_lotto_region_atomic_enter_cond != NULL) {
        _lotto_region_atomic_enter_cond(cond);
    }
}
/**
 * Leave the atomic region if the condition is met.
 *
 * @param cond condition
 */
static inline void
lotto_region_atomic_leave_cond(bool cond)
{
    if (_lotto_region_atomic_leave_cond != NULL) {
        _lotto_region_atomic_leave_cond(cond);
    }
}
/**
 * Enter the atomic region if the executing task id matches the argument.
 *
 * @param task task id
 */
static inline void
lotto_region_atomic_enter_task(task_id task)
{
    if (_lotto_region_atomic_enter_task != NULL) {
        _lotto_region_atomic_enter_task(task);
    }
}
/**
 * Leave the atomic region if the executing task id matches the argument.
 *
 * @param task task id
 */
static inline void
lotto_region_atomic_leave_task(task_id task)
{
    if (_lotto_region_atomic_leave_task != NULL) {
        _lotto_region_atomic_leave_task(task);
    }
}
/**
 * Enter the nonatomic region.
 */
static inline void
lotto_region_nonatomic_enter(void)
{
    if (_lotto_region_nonatomic_enter != NULL) {
        _lotto_region_nonatomic_enter();
    }
}
/**
 * Leave the nonatomic region.
 */
static inline void
lotto_region_nonatomic_leave(void)
{
    if (_lotto_region_nonatomic_leave != NULL) {
        _lotto_region_nonatomic_leave();
    }
}
/**
 * Enter the nonatomic region if the condition is met.
 *
 * @param cond condition
 */
static inline void
lotto_region_nonatomic_enter_cond(bool cond)
{
    if (_lotto_region_nonatomic_enter_cond != NULL) {
        _lotto_region_nonatomic_enter_cond(cond);
    }
}
/**
 * Leave the nonatomic region if the condition is met.
 *
 * @param cond condition
 */
static inline void
lotto_region_nonatomic_leave_cond(bool cond)
{
    if (_lotto_region_nonatomic_leave_cond != NULL) {
        _lotto_region_nonatomic_leave_cond(cond);
    }
}
/**
 * Enter the nonatomic region if the executing task id matches the argument.
 *
 * @param task task id
 */
static inline void
lotto_region_nonatomic_enter_task(task_id task)
{
    if (_lotto_region_nonatomic_enter_task != NULL) {
        _lotto_region_nonatomic_enter_task(task);
    }
}
/**
 * Leave the nonatomic region if the executing task id matches the argument.
 *
 * @param task task id
 */
static inline void
lotto_region_nonatomic_leave_task(task_id task)
{
    if (_lotto_region_nonatomic_leave_task != NULL) {
        _lotto_region_nonatomic_leave_task(task);
    }
}

static inline void __attribute__((unused))
lotto_region_atomic_cleanup(void *ghost)
{
    (void)ghost;
    if (_lotto_region_atomic_leave != NULL) {
        _lotto_region_atomic_leave();
    }
}

static inline void __attribute__((unused))
lotto_region_nonatomic_cleanup(void *ghost)
{
    (void)ghost;
    if (_lotto_region_nonatomic_leave != NULL) {
        _lotto_region_nonatomic_leave();
    }
}

#endif

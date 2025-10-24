/*
 */
#ifndef LOTTO_REGION_PREEMPTION_H
#define LOTTO_REGION_PREEMPTION_H
/**
 * @file region_preemption.h
 * @brief The preemption region interface.
 *
 * The preemption region enables/disables task preemptions depending on the
 * default state (controled by the the CLI flag
 * `--region-preemption-default-off`). I.e., if the flag is unset, the regions
 * behave atomically, otherwise tasks may preempt only inside regions.
 */

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t task_id;

/**
 * Enter the preemption region.
 */
void lotto_region_preemption_enter() __attribute__((weak));
/**
 * Leave the preemption region.
 */
void lotto_region_preemption_leave() __attribute__((weak));
/**
 * Enter the preemption region if the condition is met.
 *
 * @param cond condition
 */
void lotto_region_preemption_enter_cond(bool cond) __attribute__((weak));
/**
 * Leave the preemption region if the condition is met.
 *
 * @param cond condition
 */
void lotto_region_preemption_leave_cond(bool cond) __attribute__((weak));
/**
 * Enter the preemption region if the executing task id matches the argument.
 *
 * @param task task id
 */
void lotto_region_preemption_enter_task(task_id task) __attribute__((weak));
/**
 * Leave the preemption region if the executing task id matches the argument.
 *
 * @param task task id
 */
void lotto_region_preemption_leave_task(task_id task) __attribute__((weak));

#endif

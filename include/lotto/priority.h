/*
 */
#ifndef LOTTO_PRIORITY_H
#define LOTTO_PRIORITY_H
/**
 * @file priority.h
 * @brief The priority interface.
 *
 * User priorities override strategy decision: the strategy applies to available
 * tasks with the highest user priority. Initially, all priorities are zero.
 */

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t task_id;

/**
 * Sets the task priority.
 *
 * @param priority priority
 */
void lotto_priority(int64_t priority) __attribute__((weak));
/**
 * Sets the task priority if the condition is satisfied.
 *
 * @param priority priority
 * @param cond     condition
 */
void lotto_priority_cond(bool cond, int64_t priority) __attribute__((weak));
/**
 * Sets the task priority if the executing task id is equal to `task`.
 *
 * @param priority priority
 * @param task     task id
 */
void lotto_priority_task(task_id task, int64_t priority) __attribute__((weak));

#endif

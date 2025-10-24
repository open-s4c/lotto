/*
 */
#ifndef LOTTO_VELOCITY_H
#define LOTTO_VELOCITY_H
/**
 * @file velocity.h
 * @brief The task velocity interface.
 *
 * User task velocities override strategy decision: the strategy applies to a
 * given task by setting of a probability for processing of events in this task.
 * Initially this probability, or velocity, is set to 1.0. It can be set to any
 * number between 1% and 100%.
 */

#include <stdbool.h>
#include <stdint.h>

#define LOTTO_TASK_VELOCITY_MIN (1)
#define LOTTO_TASK_VELOCITY_MAX (100)

/**
 * Sets the task velocity that must be between 1 and 100.
 *
 * @param probability probability
 */
void lotto_task_velocity(int64_t probability) __attribute__((weak));

#endif

/*
 */
#ifndef LOTTO_YIELD_H
#define LOTTO_YIELD_H
/**
 * @file yield.h
 * @brief The yield interface.
 *
 * The yield interface can be used for either enforcing preemptions or adding
 * change points.
 */

#include <stdbool.h>

/**
 * Preempt to another available task. An advisory yield is interpreted as a
 * change point, i.e., the preemption decision is left to the strategy and may
 * include the current task. Otherwise, the current task is excluded from the
 * pool of next tasks.
 *
 * @param advisory whether the yield is advisory
 * @return zero
 */
int lotto_yield(bool advisory) __attribute__((weak));

#endif

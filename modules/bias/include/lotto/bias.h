/**
 * @file bias.h
 * @brief Scheduler bias interface.
 *
 * The bias module can force a simple selection policy at each mutable change
 * point. The toggle API swaps the current policy with the configured alternate
 * policy.
 */
#ifndef LOTTO_BIAS_H
#define LOTTO_BIAS_H

#include <stdint.h>

/** Bias policy identifiers. */
typedef enum bias_policy_kind {
    BIAS_POLICY_NONE = 0,
    BIAS_POLICY_CURRENT,
    BIAS_POLICY_LOWEST,
    BIAS_POLICY_HIGHEST,
} bias_policy_t;

/** Set the current bias policy. */
void lotto_bias_policy(bias_policy_t policy) __attribute__((weak));

/** Swap the current policy with the configured toggle policy. */
void lotto_bias_toggle(void) __attribute__((weak));

#endif

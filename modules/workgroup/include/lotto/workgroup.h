/**
 * @file workgroup.h
 * @brief Workgroup interface.
 */
#ifndef LOTTO_WORKGROUP_H
#define LOTTO_WORKGROUP_H

#include <stdint.h>

typedef enum workgroup_thread_start_policy_kind {
    WORKGROUP_THREAD_START_POLICY_SEQUENTIAL = 0,
    WORKGROUP_THREAD_START_POLICY_CONCURRENT,
} workgroup_thread_start_policy_t;

void _lotto_workgroup_join(void) __attribute__((weak));

static inline void
lotto_workgroup_join(void)
{
    if (_lotto_workgroup_join != NULL) {
        _lotto_workgroup_join();
    }
}

#endif

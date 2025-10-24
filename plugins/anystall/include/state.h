/*
 */
#ifndef LOTTO_STATE_ANYSTALL_H
#define LOTTO_STATE_ANYSTALL_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct anystall_config {
    marshable_t m;
    bool enabled;
    uint64_t anytask_limit;
} anystall_config_t;

anystall_config_t *anystall_config();

#endif

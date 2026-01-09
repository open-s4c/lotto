/*
 */
#ifndef LOTTO_STATE_REGION_PRREMPTION_H
#define LOTTO_STATE_REGION_PRREMPTION_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct region_preemption_config {
    marshable_t m;
    bool enabled;
    bool default_on;
} region_preemption_config_t;

region_preemption_config_t *region_preemption_config();

#endif

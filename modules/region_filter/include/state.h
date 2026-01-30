/*
 */
#ifndef LOTTO_STATE_REGION_FILTER_H
#define LOTTO_STATE_REGION_FILTER_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct region_filter {
    marshable_t m;
    bool enabled;
} region_filter_config_t;

region_filter_config_t *region_filter_config();

#endif

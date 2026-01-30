/*
 */
#ifndef LOTTO_STATE_POS_H
#define LOTTO_STATE_POS_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct pos_config {
    marshable_t m;
    uint64_t wd_threshold;
    uint64_t wd_divisor;
    bool enabled;
} pos_config_t;

pos_config_t *pos_config();

#endif

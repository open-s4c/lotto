#ifndef LOTTO_STATE_CLOCK_H
#define LOTTO_STATE_CLOCK_H

#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct clock_config {
    marshable_t m;
    uint64_t base_inc;
    uint64_t mult_inc;
    uint64_t max_gap;
    uint64_t burst_gap;
} clock_config_t;

clock_config_t *clock_config(void);

#endif

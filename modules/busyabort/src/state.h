#ifndef LOTTO_BUSYABORT_STATE_H
#define LOTTO_BUSYABORT_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct busyabort_config {
    marshable_t m;
    bool enabled;
    uint64_t threshold;
} busyabort_config_t;

busyabort_config_t *busyabort_config(void);

#endif

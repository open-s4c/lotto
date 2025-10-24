/*
 */
#ifndef LOTTO_STATE_WATCHDOG_H
#define LOTTO_STATE_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct watchdog_config {
    marshable_t m;
    bool enabled;
    uint64_t budget;
} watchdog_config_t;

watchdog_config_t *watchdog_config();

#endif

/*
 */
#ifndef LOTTO_STATE_YIELD_H
#define LOTTO_STATE_YIELD_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct yield_config {
    marshable_t m;
    bool enabled;
    bool advisory;
} yield_config_t;

yield_config_t *yield_config();

#endif

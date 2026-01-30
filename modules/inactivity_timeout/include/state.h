/*
 */
#ifndef LOTTO_STATE_INACTIVITY_TIMEOUT_H
#define LOTTO_STATE_INACTIVITY_TIMEOUT_H

#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct inactivity_timeout_config {
    marshable_t m;
    uint64_t alarm;
} inactivity_timeout_config_t;

inactivity_timeout_config_t *inactivity_timeout_config();

#endif

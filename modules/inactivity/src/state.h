/**
 * @file state.h
 * @brief Inactivity module state declarations.
 */
#ifndef LOTTO_STATE_INACTIVITY_H
#define LOTTO_STATE_INACTIVITY_H

#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct inactivity_config {
    marshable_t m;
    uint64_t alarm;
} inactivity_config_t;

inactivity_config_t *inactivity_config();

#endif

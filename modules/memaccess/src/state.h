/**
 * @file state.h
 * @brief Memaccess module state declarations.
 */
#ifndef LOTTO_STATE_MEMACCESS_H
#define LOTTO_STATE_MEMACCESS_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct memaccess_config {
    marshable_t m;
    bool enabled;
} memaccess_config_t;

memaccess_config_t *memaccess_config();

#endif

#ifndef LOTTO_STATE_ATOMIC_H
#define LOTTO_STATE_ATOMIC_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct atomic_config {
    marshable_t m;
    bool enabled;
} atomic_config_t;

atomic_config_t *atomic_config();

#endif

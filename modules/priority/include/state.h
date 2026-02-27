#ifndef LOTTO_STATE_PRIORITY_H
#define LOTTO_STATE_PRIORITY_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct priority_config {
    marshable_t m;
    bool enabled;
} priority_config_t;

priority_config_t *priority_config();

#endif

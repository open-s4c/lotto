#ifndef LOTTO_MODULES_TIME_STATE_H
#define LOTTO_MODULES_TIME_STATE_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct time_config {
    marshable_t m;
    bool enabled;
} time_config_t;

time_config_t *time_config(void);

#endif

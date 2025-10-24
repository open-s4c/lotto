/*
 */
#ifndef LOTTO_STATE_DROP_H
#define LOTTO_STATE_DROP_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct drop_config {
    marshable_t m;
    bool enabled;
} drop_config_t;

drop_config_t *drop_config();

#endif

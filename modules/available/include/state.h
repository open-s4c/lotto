/*
 */
#ifndef LOTTO_STATE_AVAILABLE_H
#define LOTTO_STATE_AVAILABLE_H

#include <lotto/base/tidset.h>

typedef struct available_config {
    marshable_t m;
    bool enabled;
} available_config_t;

tidset_t *get_available_tasks();
available_config_t *available_config();

#endif

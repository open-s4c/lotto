/*
 */
#ifndef LOTTO_STATE_FILTERING_H
#define LOTTO_STATE_FILTERING_H

#include <limits.h>
#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct filtering_config {
    marshable_t m;
    bool enabled;
    char filename[PATH_MAX];
} filtering_config_t;

filtering_config_t *filtering_config();

#endif

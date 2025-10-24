/*
 */
#ifndef LOTTO_STATE_CAPTURE_GROUP_H
#define LOTTO_STATE_CAPTURE_GROUP_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

#define CAPTURED_FUNCTIONS_MAX_LENGTH 10000

typedef struct capture_group_config {
    marshable_t m;
    bool atomic;
    bool check;
    char path[PATH_MAX];
    uint64_t group_threshold;
    char functions[CAPTURED_FUNCTIONS_MAX_LENGTH];
} capture_group_config_t;

capture_group_config_t *capture_group_config();

#endif

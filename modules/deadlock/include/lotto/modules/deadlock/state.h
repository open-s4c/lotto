#ifndef LOTTO_STATE_DEADLOCK_H
#define LOTTO_STATE_DEADLOCK_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct deadlock_config {
    marshable_t m;
    bool enabled;
    bool lost_resource_check;
    bool extra_release_check;
} deadlock_config_t;

deadlock_config_t *deadlock_config();

#endif

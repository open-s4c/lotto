#ifndef LOTTO_STATE_RWLOCK_H
#define LOTTO_STATE_RWLOCK_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct _config {
    marshable_t m;
    bool enabled;
} rwlock_config_t;

rwlock_config_t *rwlock_config();

#endif

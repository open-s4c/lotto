#ifndef LOTTO_STATE_MUTEX_H
#define LOTTO_STATE_MUTEX_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct mutex_config {
    marshable_t m;
    bool enabled;
    bool deadlock_check;
} mutex_config_t;

mutex_config_t *mutex_config(void);

#endif /* LOTTO_STATE_MUTEX_H */

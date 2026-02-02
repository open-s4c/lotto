#ifndef LOTTO_STATE_TASK_VELOCITY_H
#define LOTTO_STATE_TASK_VELOCITY_H

#include <stdbool.h>

#include <lotto/base/marshable.h>

typedef struct task_velocity_config {
    marshable_t m;
    bool enabled;
} task_velocity_config_t;

task_velocity_config_t *task_velocity_config();

#endif

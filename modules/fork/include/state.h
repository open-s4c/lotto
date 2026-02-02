/*
 */
#ifndef LOTTO_STATE_FORK_EXECVE_H
#define LOTTO_STATE_FORK_EXECVE_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct fork_execve_config {
    marshable_t m;
    bool enabled;
} fork_execve_config_t;

fork_execve_config_t *fork_execve_config();

#endif

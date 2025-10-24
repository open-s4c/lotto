/*
 */
#ifndef LOTTO_STATE_RECONSTRUCT_H
#define LOTTO_STATE_RECONSTRUCT_H

#include <lotto/base/log.h>

typedef struct {
    marshable_t m;
    bool enabled;
    uint64_t tid;
    uint64_t id;
    uint64_t data;
} reconstruct_config_t;

reconstruct_config_t *reconstruct_config();
log_t *reconstruct_get_log();

#endif

/*
 */
#ifndef LOTTO_STATE_PCT_H
#define LOTTO_STATE_PCT_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidmap.h>

typedef struct pct_config {
    marshable_t m;
    bool enabled;
    bool initd;

    uint64_t k;
    uint64_t d;

    uint64_t chpts;
} pct_config_t;

typedef struct {
    tiditem_t t;
    uint64_t priority;
} task_t;

typedef struct pct_state {
    marshable_t m;
    tidmap_t map;

    char payload[0];
    struct {
        uint64_t chpts;
        uint64_t kmax;
        uint64_t nmax;
    } counts;

} pct_state_t;

pct_config_t *pct_config();
pct_state_t *pct_state();

#endif

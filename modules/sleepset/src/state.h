/**
 * @file state.h
 * @brief Sleepset module state declarations.
 */
#ifndef LOTTO_STATE_SLEEPSET_H
#define LOTTO_STATE_SLEEPSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidmap.h>
#include <lotto/base/tidset.h>

typedef struct sleepset_config {
    marshable_t m;
    bool enabled;
} sleepset_config_t;

typedef enum sleepset_access_kind {
    SLEEPSET_ACCESS_NONE = 0,
    SLEEPSET_ACCESS_READ,
    SLEEPSET_ACCESS_WRITE,
} sleepset_access_kind_t;

typedef struct sleepset_access {
    bool valid;
    sleepset_access_kind_t kind;
    uintptr_t addr;
    size_t size;
} sleepset_access_t;

typedef struct sleepset_task_access {
    tiditem_t t;
    sleepset_access_t access;
} sleepset_task_access_t;

typedef struct sleepset_state {
    tidset_t sleeping;
    tidset_t prev_enabled;
    tidset_t cur_enabled;
    tidset_t next_sleeping;
    tidmap_t accesses;
} sleepset_state_t;

sleepset_config_t *sleepset_config();
sleepset_state_t *sleepset_state();
void sleepset_reset(void);

#endif

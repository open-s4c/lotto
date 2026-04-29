#ifndef LOTTO_STATE_IMPASSE_H
#define LOTTO_STATE_IMPASSE_H

#include <stdbool.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidset.h>

typedef struct impasse_config {
    marshable_t m;
    bool enabled;
} impasse_config_t;

impasse_config_t *impasse_config();

/** Non-blocked or soft-blocked tasks. */
const tidset_t *available_or_soft_blocked_tasks();

#endif

#ifndef LOTTO_STATE_IMPASSE_H
#define LOTTO_STATE_IMPASSE_H

#include <lotto/base/tidset.h>

/** Non-blocked or soft-blocked tasks. */
const tidset_t *available_or_soft_blocked_tasks();

#endif

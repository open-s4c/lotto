/**
 * @file poll.h
 * @brief Poll module declarations for poll.
 */
#ifndef LOTTO_MODULES_POLL_POLL_H
#define LOTTO_MODULES_POLL_POLL_H

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/modules/poll/events.h>

bool lotto_dbg_poll_is_waiting(task_id id);
const tidset_t *lotto_dbg_poll_waiters(void);

#endif

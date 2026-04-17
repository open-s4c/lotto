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
#include <lotto/modules/poll/events.h>

bool _lotto_poll_is_waiting(task_id id);
void _lotto_poll_print_waiters(void);

#endif

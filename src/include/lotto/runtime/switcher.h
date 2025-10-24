/*
 */
#ifndef LOTTO_SWITCHER_H
#define LOTTO_SWITCHER_H

#include <stdbool.h>

#include <lotto/base/task_id.h>
#include <lotto/sys/now.h>

typedef enum switcher_status {
    SWITCHER_CONTINUE,
    SWITCHER_CHANGED,
    SWITCHER_ABORTED
} switcher_status_t;

switcher_status_t switcher_yield(task_id id, bool (*any_task_filter)(task_id));
void switcher_wake(task_id id, nanosec_t slack);
void switcher_abort(void);

#endif

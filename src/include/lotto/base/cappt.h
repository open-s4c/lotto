/*
 */
#ifndef LOTTO_CAPPT_H
#define LOTTO_CAPPT_H

/*******************************************************************************
 * @file cappt.h
 * @brief represents a capture point within the engine
 *
 ******************************************************************************/

#include <stdbool.h>

#include <lotto/base/clk.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/sys/assert.h>

enum selector {
    SELECTOR_UNDEFINED = 0,
    SELECTOR_RANDOM,
    SELECTOR_FIRST,
};

typedef bool(*any_task_filter_f)(task_id);

#define MAX_ANY_TASK_FILTERS 16

typedef struct {
    int n;
    any_task_filter_f val[MAX_ANY_TASK_FILTERS];
} any_task_filters;

#define NO_ANY_TASK_FILTERS ((any_task_filters){0})

typedef struct cappt {
    const clk_t clk;    /**< Current sequencer clock */
    bool is_chpt;       /**< Whether the capture point becomes a change point */
    task_id next;       /**< Next task to schedule if event is a chpt */
    bool readonly;      /**< Marks event as read-only */
    bool skip;          /**< Filter triggered */
    tidset_t tset;      /**< Set of available tasks if event becomes a chpt */
    tidset_t unblocked; /**< Set of tasks to be unblocked */
    reason_t reason;    /**< Reason event transition */
    enum selector selector; /**< Next task selector (initially undefined) */
    bool should_record;     /**< Whether the event must be recorded */
    bool filter_less;       /**< Filtering handler should filter less */
    bool replay;            /**< Whether the event is being replayed */
    any_task_filters any_task_filters; /** returns true if the task may wake as ANY_TASK */
} event_t;

void cappt_add_any_task_filter(event_t *e, any_task_filter_f f);

#endif

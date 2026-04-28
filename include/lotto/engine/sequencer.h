/**
 * @file sequencer.h
 * @brief Engine declarations for sequencer.
 */
#ifndef LOTTO_SEQUENCER_H
#define LOTTO_SEQUENCER_H

#include <stdbool.h>

#include <lotto/base/clk.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>
#include <lotto/base/tidset.h>
#include <lotto/check.h>
#include <lotto/engine/plan.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>

enum selector {
    SELECTOR_UNDEFINED = 0,
    SELECTOR_RANDOM,
    SELECTOR_FIRST,
};

typedef struct sequencer_decision {
    const clk_t clk;
    bool is_chpt;
    task_id next;
    bool readonly;
    bool skip;
    tidset_t tset;
    tidset_t unblocked;
    reason_t reason;
    enum selector selector;
    bool should_record;
    bool filter_less;
    bool replay;
    bool (*any_task_filter)(task_id);
} sequencer_decision;

typedef sequencer_decision event_t;

typedef void (*handle_f)(const capture_point *cp, sequencer_decision *e);

/**
 * Terminates sequencer
 */
void sequencer_fini(const capture_point *cp, reason_t reason);

/**
 * Returns an action for a given captured context.
 *
 * The action has to be fulfilled following the expected contract.
 */
struct plan sequencer_capture(const capture_point *cp);

/**
 * Informs sequencer that task is resuming after an ACTION_YIELD.
 */
void sequencer_resume(const capture_point *cp);

/**
 * Informs the sequencer that a previously blocking semantic ingress event has
 * completed for the task.
 */
void sequencer_return(const capture_point *cp);

/**
 * Returns the current clock
 */
clk_t sequencer_get_clk();
void sequencer_set_clk(clk_t clk);

/* Run a handler for each sequencer capture event. */
#define ON_SEQUENCER_CAPTURE(HANDLE)                                           \
    LOTTO_SUBSCRIBE_SEQUENCER_CAPTURE(ANY_EVENT, {                             \
        const capture_point *cp = EVENT_PAYLOAD(cp);                           \
        sequencer_decision *e   = cp->decision;                                \
        HANDLE(cp, e);                                                         \
        if (e->skip)                                                           \
            return PS_STOP_CHAIN;                                              \
    })

/* Run a handler for each sequencer resume event. */
#define ON_SEQUENCER_RESUME(HANDLE)                                            \
    LOTTO_SUBSCRIBE_SEQUENCER_RESUME(ANY_EVENT, {                              \
        const capture_point *cp = EVENT_PAYLOAD(cp);                           \
        sequencer_decision *e   = cp->decision;                                \
        HANDLE(cp, e);                                                         \
        if (e && e->skip)                                                      \
            return PS_STOP_CHAIN;                                              \
    })

#endif

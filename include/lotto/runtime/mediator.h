/**
 * @file mediator.h
 * @brief Runtime declarations for mediator.
 */
#ifndef LOTTO_MEDIATOR_H
#define LOTTO_MEDIATOR_H

#include <dice/types.h>
#include <lotto/engine/plan.h>
#include <lotto/runtime/capture_point.h>

typedef enum mediator_optimization {
    MEDIATOR_OPTIMIZATION_NONE,
    MEDIATOR_OPTIMIZATION_SAME_TASK_EXCEPT_ANY,
    MEDIATOR_OPTIMIZATION_SAME_TASK,
} mediator_optimization_t;

typedef struct mediator {
    task_id id;
    struct plan plan;
    bool finito;
    mediator_optimization_t optimization;

    struct mediator_stats {
        uint64_t count1;
        uint64_t count2;
    } stats;
} mediator_t;

typedef enum mediator_status {
    MEDIATOR_OK,
    MEDIATOR_ABORT,
    MEDIATOR_SHUTDOWN,
} mediator_status_t;

/* return
 * - true if capture point became a change point
 *   (and resume shall be called)
 * - false if capture point ignore, just continue execution
 */
bool mediator_capture(mediator_t *m, capture_point *cp);

/* Resume executes the remainder of the current plan for the given semantic
 * capture point.
 *
 * On first access, mediator_get(..., true) bootstraps engine resume before any
 * semantic ingress is captured for the task.
 */
mediator_status_t mediator_resume(mediator_t *m, capture_point *cp);

/* Complete a previously blocking semantic ingress event once the task becomes
 * runnable again.
 */
void mediator_return(mediator_t *m, capture_point *cp);

void mediator_fini(mediator_t *m);

bool mediator_is_any_task(mediator_t *m);

/* returns true if between capture and resume */
bool mediator_in_capture(const mediator_t *m);

/* return the current mediator, optionally bootstrapping engine resume on first
 * access */
mediator_t *mediator_get(metadata_t *md, bool bootstrap);

#endif

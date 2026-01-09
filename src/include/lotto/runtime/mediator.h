/*
 */
#ifndef LOTTO_MEDIATOR_H
#define LOTTO_MEDIATOR_H

#include <lotto/base/context.h>
#include <lotto/base/plan.h>

typedef enum mediator_optimization {
    MEDIATOR_OPTIMIZATION_NONE,
    MEDIATOR_OPTIMIZATION_SAME_TASK_EXCEPT_ANY,
    MEDIATOR_OPTIMIZATION_SAME_TASK,
} mediator_optimization_t;

typedef enum mediator_registration {
    MEDIATOR_REGISTRATION_NONE,
    MEDIATOR_REGISTRATION_NEED,
    MEDIATOR_REGISTRATION_EXEC,
    MEDIATOR_REGISTRATION_DONE,
} mediator_registration_t;

#define MEDIATOR_VALUE_CAP 1024

typedef struct mediator {
    task_id id;
    plan_t plan;
    bool finito;
    mediator_optimization_t optimization;
    mediator_registration_t registration_status;

    struct mediator_stats {
        uint64_t count1;
        uint64_t count2;
    } stats;

    struct mediator_guards {
        int capture;
        int detach;
    } guards;

    size_t nvalues;
    struct mediator_value {
        pthread_key_t key;
        void *value;
    } values[MEDIATOR_VALUE_CAP];
} mediator_t;

typedef enum mediator_status {
    MEDIATOR_OK,
    MEDIATOR_ABORT,
    MEDIATOR_SHUTDOWN,
} mediator_status_t;

/*
 * creates a new mediator
 */
mediator_t *mediator_init();

/* return
 * - true if capture point became a change point
 *   (and resume shall be called)
 * - false if capture point ignore, just continue execution
 */
bool mediator_capture(mediator_t *m, context_t *ctx);

/* first call by each task must be with CAT_NONE
 * mediation will be then reset when CAT_NONE is given.
 *
 * resume decides whether execution should continue or not
 *
 */
mediator_status_t mediator_resume(mediator_t *m, context_t *ctx);

/* shall be called once task available again. */
void mediator_return(mediator_t *m, context_t *ctx);

void mediator_fini(mediator_t *m);

bool mediator_is_any_task(mediator_t *m);

/* returns true if it is the first detach call.
 */
bool mediator_detach(mediator_t *m);

/* returns true if not yet all nesting is over */
bool mediator_attach(mediator_t *m);

/* return true if between detach and attach */
bool mediator_detached(const mediator_t *m);

/* returns true if between capture and resume */
bool mediator_in_capture(const mediator_t *m);

/* return per-thread mediator data memory pointer */
mediator_t *mediator_get_data(bool new_task);

/* disallows new tasks to register */
void mediator_disable_registration();

#endif

#ifndef LOTTO_RUNTIME_MEDIATOR_H
#define LOTTO_RUNTIME_MEDIATOR_H

#include <stdbool.h>

#include <dice/types.h>

#include <lotto/base/context.h>
#include <lotto/base/plan.h>
#include <lotto/base/reason.h>
#include <lotto/base/task_id.h>

typedef enum mediator_registration {
    MEDIATOR_REGISTRATION_NONE,
    MEDIATOR_REGISTRATION_DONE,
} mediator_registration_t;

typedef enum mediator_status {
    MEDIATOR_OK,
    MEDIATOR_ABORT,
    MEDIATOR_SHUTDOWN,
} mediator_status_t;

typedef struct mediator {
    task_id id;
    plan_t plan;
    mediator_registration_t registration_status;
    int capture_depth;
    int detach_depth;
    bool finito;
    bool has_pending_reason;
    reason_t pending_reason;
} mediator_t;

mediator_t *mediator_tls(metadata_t *md);
bool mediator_capture(mediator_t *m, context_t *ctx);
mediator_status_t mediator_resume(mediator_t *m, context_t *ctx);
void mediator_return(mediator_t *m, context_t *ctx);
void mediator_fini(mediator_t *m);
bool mediator_detached(const mediator_t *m);
bool mediator_detach(mediator_t *m);
bool mediator_attach(mediator_t *m);
bool mediator_in_capture(const mediator_t *m);
void mediator_disable_registration(void);

#endif /* LOTTO_RUNTIME_MEDIATOR_H */

#include <dice/chains/capture.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/fork/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>

typedef struct {
    const char *func;
} fork_event_t;

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_FORK_EXECVE, {
    fork_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp = {
        .chain_id = CAPTURE_BEFORE,
        .type_id  = EVENT_FORK_EXECVE,
        .blocking = true,
        .payload  = ev,
        .func     = ev->func,
    };
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_FORK_EXECVE, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_FORK_EXECVE, {
    fork_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp = {
        .chain_id = CAPTURE_AFTER,
        .type_id  = EVENT_FORK_EXECVE,
        .blocking = true,
        .payload  = ev,
        .func     = ev->func,
    };
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_FORK_EXECVE, &cp, md);
    return PS_OK;
})

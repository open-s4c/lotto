#include <stdbool.h>

#include <dice/chains/capture.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/yield/events.h>
#include <lotto/runtime/capture_point.h>

typedef struct yield_event {
    bool advisory;
} yield_event_t;

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_YIELD_SCHED, {
    capture_point cp = {
        .type_id = EVENT_YIELD_USER,
        .func    = "sched_yield",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_YIELD_USER, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_YIELD_LOTTO, {
    yield_event_t *ev    = EVENT_PAYLOAD(event);
    type_id ingress_type = ev->advisory ? EVENT_YIELD_SYS : EVENT_YIELD_USER;
    capture_point cp     = {
            .type_id = ingress_type,
            .payload = ev,
            .func    = "lotto_yield",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, ingress_type, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_YIELD_USER, {
    capture_point cp = {
        .type_id = EVENT_YIELD_USER,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_YIELD_USER, &cp, md);
    return PS_OK;
})

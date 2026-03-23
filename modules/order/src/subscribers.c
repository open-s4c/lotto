#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/order/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>

typedef struct {
    const char *func;
    uint64_t order;
} order_event_t;

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_ORDER, {
    order_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp  = {
        .src_chain = CAPTURE_BEFORE,
        .src_type  = EVENT_ORDER,
        .payload   = ev,
        .func      = ev->func,
        .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_ORDER, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_ORDER, {
    order_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp  = {
        .src_chain = CAPTURE_AFTER,
        .src_type  = EVENT_ORDER,
        .payload   = ev,
        .func      = ev->func,
        .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_ORDER, &cp, md);
    return PS_OK;
})

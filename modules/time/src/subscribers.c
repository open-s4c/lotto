#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/time/events.h>
#include <lotto/modules/yield/events.h>
#include <lotto/runtime/capture_point.h>

typedef struct {
    const char *func;
} time_yield_event_t;

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_TIME_YIELD, {
    time_yield_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp       = {
              .type_id = EVENT_YIELD_SYS,
              .payload = ev,
              .func    = ev->func,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_YIELD_SYS, &cp, md);
    return PS_OK;
})

#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/time/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/module_events.h>

typedef struct {
    const char *func;
} time_yield_event_t;

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_TIME_YIELD, {
    time_yield_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp       = {
              .type_id = EVENT_SYS_YIELD,
              .payload = ev,
              .func    = ev->func,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_SYS_YIELD, &cp, md);
    return PS_OK;
})

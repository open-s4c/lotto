#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/sleep/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SLEEP_YIELD, {
    sleep_yield_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp        = {
               .type_id = EVENT_SLEEP_YIELD,
               .payload = ev,
               .func    = ev->func,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_SLEEP_YIELD, &cp, md);
    return PS_OK;
})

#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/priority/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_PRIORITY, {
    priority_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp     = {
            .type_id = EVENT_PRIORITY,
            .payload = ev,
            .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_PRIORITY, &cp, md);
    return PS_OK;
})

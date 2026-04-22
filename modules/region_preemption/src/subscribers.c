#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/region_preemption/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_REGION_PREEMPTION, {
    region_preemption_event_t *ev = EVENT_PAYLOAD(event);
    type_id type     = ev->in_region ? EVENT_REGION_PREEMPTION_IN
                                     : EVENT_REGION_PREEMPTION_OUT;
    capture_point cp = {
        .payload = ev,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, type, &cp, md);
    return PS_OK;
})

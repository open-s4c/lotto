#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_RSRC_ACQUIRING, {
    rsrc_event_t *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .src_chain = chain,
        .src_type  = type,
        .payload   = ev,
        .func      = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RSRC_ACQUIRING, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_RSRC_RELEASED, {
    rsrc_event_t *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .src_chain = chain,
        .src_type  = type,
        .payload   = ev,
        .func      = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RSRC_RELEASED, &cp, md);
    return PS_OK;
})

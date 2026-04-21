#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/bias/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_BIAS_POLICY, {
    bias_policy_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp        = {
               .type_id = EVENT_BIAS_POLICY,
               .payload = ev,
               .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_BIAS_POLICY, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_BIAS_TOGGLE, {
    capture_point cp = {
        .type_id = EVENT_BIAS_TOGGLE,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_BIAS_TOGGLE, &cp, md);
    return PS_OK;
})

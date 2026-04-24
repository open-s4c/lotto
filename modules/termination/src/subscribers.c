#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/termination/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_SUCCESS, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_SUCCESS,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_SUCCESS, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_FAIL, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_FAIL,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_FAIL, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_STOP, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_STOP,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_STOP, &cp, md);
    return PS_OK;
})

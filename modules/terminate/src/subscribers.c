#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/terminate/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_SUCCESS, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_SUCCESS,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_SUCCESS, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_FAILURE, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_FAILURE,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_FAILURE, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_TERMINATE_ABANDON, {
    capture_point cp = {
        .type_id = EVENT_TERMINATE_ABANDON,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TERMINATE_ABANDON, &cp, md);
    return PS_OK;
})

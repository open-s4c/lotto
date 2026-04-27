#include "state.h"
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/workgroup/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>

static inline enum ps_err
workgroup_filter_(metadata_t *md, chain_id chain, type_id type)
{
    return workgroup_should_filter(md, chain, type) ? PS_STOP_CHAIN : PS_OK;
}

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_TASK_INIT, {
    workgroup_on_task_init(md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_TASK_FINI, {
    workgroup_on_task_fini(md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_WORKGROUP_JOIN, {
    capture_point cp = {
        .type_id = EVENT_WORKGROUP_JOIN,
        .func    = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_WORKGROUP_JOIN, &cp, md);
    workgroup_clear_current(md);
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, ANY_EVENT,
             { return workgroup_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_BEFORE, ANY_EVENT,
             { return workgroup_filter_(md, chain, type); })

PS_SUBSCRIBE(CHAIN_INGRESS_AFTER, ANY_EVENT,
             { return workgroup_filter_(md, chain, type); })

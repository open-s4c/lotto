#include <dice/chains/capture.h>
#include <dice/events/cxa.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/cxa/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_CXA_GUARD_ACQUIRE, {
    struct __cxa_guard_acquire_event *ev = EVENT_PAYLOAD(event);
    capture_point cp                     = {
                            .src_chain = CAPTURE_BEFORE,
                            .src_type  = EVENT_CXA_GUARD_CALL,
                            .payload   = ev,
                            .pc        = (uintptr_t)ev->pc,
                            .func      = "__cxa_guard_acquire",
                            .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, type, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_CXA_GUARD_ACQUIRE, {
    capture_point cp = {
        .src_chain = CAPTURE_AFTER,
        .src_type  = EVENT_CXA_GUARD_CALL,
        .payload   = NULL,
        .func      = "__cxa_guard_acquire",
        .blocking  = true,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, type, &cp, md);
    return PS_OK;
})

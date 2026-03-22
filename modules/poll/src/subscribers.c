#include "poll.h"
#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/poll/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_POLL, {
    poll_event_t *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .src_chain = chain,
        .src_type  = type,
        .payload   = ev,
        .func      = "poll",
        .blocking  = ev->args->timeout != 0,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_POLL, &cp, md);
    return PS_OK;
})

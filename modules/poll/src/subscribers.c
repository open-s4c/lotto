#include "poll.h"
#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/poll/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>

static int
poll_default_return_(struct pollfd *fds, nfds_t nfds, int timeout)
{
    (void)fds;
    (void)nfds;
    (void)timeout;
    return 0;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_POLL, {
    struct poll_event *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = ev,
        .func     = "poll",
        .pc       = (uintptr_t)ev->pc,
        .blocking = false,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_POLL, &cp, md);
    ev->func     = poll_default_return_;
    ev->ret_save = ev->ret;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_POLL, {
    struct poll_event *ev = EVENT_PAYLOAD(event);
    ev->ret               = ev->ret_save;
    return PS_OK;
})

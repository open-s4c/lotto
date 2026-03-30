#include "events.h"
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/unsafe/concurrent.h>

FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_DETACH)
FORWARD_CAPTURE_TO_INGRESS(EVENT, EVENT_ATTACH)

void
_lotto_concurrent_enter(void)
{
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_DETACH,
        .func     = "lotto_concurrent_enter",
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_DETACH, &cp, 0);
}

void
_lotto_concurrent_leave(void)
{
    capture_point cp = {
        .chain_id = INTERCEPT_EVENT,
        .type_id  = EVENT_ATTACH,
        .func     = "lotto_concurrent_leave",
    };
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ATTACH, &cp, 0);
}

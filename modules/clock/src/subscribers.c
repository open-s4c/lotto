#include <dice/chains/capture.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/clock/events.h>
#include <lotto/runtime/capture_point.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_LOTTO_CLOCK, {
    struct lotto_clock_event *ev = EVENT_PAYLOAD(event);
    capture_point cp             = {
                    .type_id   = EVENT_LOTTO_CLOCK,
                    .blocking  = false,
                    .pc        = (uintptr_t)ev->pc,
                    .payload   = ev,
                    .func      = "lotto_clock_read",
                    .func_addr = (uintptr_t)lotto_clock_read,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_LOTTO_CLOCK, &cp, md);
    return PS_OK;
})

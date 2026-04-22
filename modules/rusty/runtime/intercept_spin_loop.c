
#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/await_while.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/rusty/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>

void
intercept_spin_start()
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUSTY_SPIN_START, NULL, 0);
}

void
intercept_spin_end(uint32_t cond)
{
    spin_end_event_t ev = {.cond = cond};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUSTY_SPIN_END, &ev, 0);
}

// strong definition
void
_lotto_spin_start()
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUSTY_SPIN_START, NULL, 0);
}

void
_lotto_spin_end(uint32_t cond)
{
    spin_end_event_t ev = {.cond = cond};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUSTY_SPIN_END, &ev, 0);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_RUSTY_SPIN_START, {
    capture_point cp = {
        .type_id = EVENT_RUSTY_SPIN_START,
        .func    = "spin_start",
        .payload = NULL,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RUSTY_SPIN_START, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_RUSTY_SPIN_END, {
    spin_end_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp     = {
            .type_id = EVENT_RUSTY_SPIN_END,
            .func    = "spin_end",
            .payload = ev,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_RUSTY_SPIN_END, &cp, md);
    return PS_OK;
})

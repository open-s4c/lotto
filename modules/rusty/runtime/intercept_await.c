#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/await.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/rusty/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>

void
intercept_await(void *addr)
{
    await_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_AWAIT, &ev, 0);
}

// strong definition
void
_lotto_await(void *addr)
{
    await_event_t ev = {.addr = addr};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_AWAIT, &ev, 0);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_AWAIT, {
    await_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp  = {
         .type_id = EVENT_AWAIT,
         .func    = "await",
         .payload = ev,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_AWAIT, &cp, md);
    return PS_OK;
})

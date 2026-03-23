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

PS_ADVERTISE_TYPE(EVENT_AWAIT)

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
    context_origin ctx = *ctx_origin(.self = self_md(), .func = __func__);
    await_event_t ev   = {.addr = addr};
    capture_point cp   = {.src_type = EVENT_AWAIT, .payload = &ev};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MODULE_INTERCEPT, &cp, (metadata_t *)&ctx);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_AWAIT, {
    await_event_t *ev = EVENT_PAYLOAD(event);
    _lotto_await(ev->addr);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_MODULE_INTERCEPT, {
    const context_origin *origin = (const context_origin *)md;
    capture_point *cp            = (capture_point *)event;

    if (cp->src_type != EVENT_AWAIT) {
        return PS_OK;
    }

    runtime_ingress_module_submit_event(origin, cp, EVENT_AWAIT);
    return PS_OK;
})

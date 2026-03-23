
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

PS_ADVERTISE_TYPE(EVENT_SPIN_START)
PS_ADVERTISE_TYPE(EVENT_SPIN_END)

void
intercept_spin_start()
{
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SPIN_START, NULL, 0);
}

void
intercept_spin_end(uint32_t cond)
{
    spin_end_event_t ev = {.cond = cond};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SPIN_END, &ev, 0);
}

// strong definition
void
_lotto_spin_start()
{
    context_origin ctx = *ctx_origin(.self = self_md(), .func = __FUNCTION__);
    capture_point cp   = {.src_type = EVENT_SPIN_START, .payload = NULL};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MODULE_INTERCEPT, &cp, (metadata_t *)&ctx);
}

void
_lotto_spin_end(uint32_t cond)
{
    context_origin ctx  = *ctx_origin(.self = self_md(), .func = __FUNCTION__);
    spin_end_event_t ev = {.cond = cond};
    capture_point cp    = {.src_type = EVENT_SPIN_END, .payload = &ev};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_MODULE_INTERCEPT, &cp, (metadata_t *)&ctx);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SPIN_START, {
    (void)event;
    _lotto_spin_start();
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SPIN_END, {
    spin_end_event_t *ev = EVENT_PAYLOAD(event);
    _lotto_spin_end(ev->cond);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_MODULE_INTERCEPT, {
    const context_origin *origin = (const context_origin *)md;
    capture_point *cp            = (capture_point *)event;

    switch (cp->src_type) {
        case EVENT_SPIN_START:
            runtime_ingress_module_submit_event(origin, cp, EVENT_SPIN_START);
            break;
        case EVENT_SPIN_END:
            runtime_ingress_module_submit_event(origin, cp, EVENT_SPIN_END);
            break;
        default:
            break;
    }
    return PS_OK;
})

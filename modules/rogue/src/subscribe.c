#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/base/context.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/rogue/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/unsafe/rogue.h>

typedef struct {
    const char *func;
} rogue_event_t;

PS_ADVERTISE_TYPE(EVENT_ROGUE)

void
_lotto_region_rogue_enter()
{
    rogue_event_t ev = {.func = __FUNCTION__};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ROGUE, &ev, 0);
}

void
_lotto_region_rogue_leave()
{
    rogue_event_t ev = {.func = __FUNCTION__};
    PS_PUBLISH(INTERCEPT_AFTER, EVENT_ROGUE, &ev, 0);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_ROGUE, {
    rogue_event_t *ev  = EVENT_PAYLOAD(event);
    context_origin ctx = *ctx_origin(.self = self_md(), .func = ev->func);
    capture_point cp   = {.type = EVENT_TASK_BLOCK,
                        .src_type = EVENT_ROGUE,
                        .blocking = true,
                        .payload = ev};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_TASK_BLOCK, &cp, (metadata_t *)&ctx);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_ROGUE, {
    rogue_event_t *ev  = EVENT_PAYLOAD(event);
    context_origin ctx = *ctx_origin(.self = self_md(), .func = ev->func);
    capture_point cp   = {.type = EVENT_TASK_BLOCK,
                        .src_type = EVENT_ROGUE,
                        .blocking = true,
                        .payload = ev};
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_TASK_BLOCK, &cp, (metadata_t *)&ctx);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, EVENT_TASK_BLOCK, {
    const context_origin *origin = (const context_origin *)md;
    capture_point *cp            = (capture_point *)event;

    if (cp->src_type != EVENT_ROGUE) {
        return PS_OK;
    }

    runtime_ingress_event(origin, EVENT_TASK_BLOCK, cp);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_INGRESS_AFTER, EVENT_TASK_BLOCK, {
    const context_origin *origin = (const context_origin *)md;
    capture_point *cp            = (capture_point *)event;

    if (cp->src_type != EVENT_ROGUE) {
        return PS_OK;
    }

    runtime_ingress_event_after(origin, EVENT_TASK_BLOCK, cp);
    return PS_OK;
})

#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/qemu/trap.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_DEADLOCK_RSRC_ACQUIRING, {
    rsrc_event_t *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = ev,
        .func     = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_DEADLOCK_RSRC_ACQUIRING, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_DEADLOCK_RSRC_RELEASED, {
    rsrc_event_t *ev = EVENT_PAYLOAD(event);

    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = ev,
        .func     = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_DEADLOCK_RSRC_RELEASED, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_DEADLOCK_RSRC_ACQUIRING, {
    capture_point *trap_cp      = EVENT_PAYLOAD(event);
    struct lotto_trap_event *trap_ev = trap_cp ? trap_cp->payload : NULL;
    rsrc_event_t rsrc_ev        = {
               .addr = trap_ev ? (void *)trap_ev->regs[1] : NULL,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = &rsrc_ev,
        .func     = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_DEADLOCK_RSRC_ACQUIRING, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CHAIN_LOTTO_TRAP, EVENT_DEADLOCK_RSRC_RELEASED, {
    capture_point *trap_cp      = EVENT_PAYLOAD(event);
    struct lotto_trap_event *trap_ev = trap_cp ? trap_cp->payload : NULL;
    rsrc_event_t rsrc_ev        = {
               .addr = trap_ev ? (void *)trap_ev->regs[1] : NULL,
    };
    capture_point cp = {
        .chain_id = chain,
        .type_id  = type,
        .payload  = &rsrc_ev,
        .func     = __FUNCTION__,
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_DEADLOCK_RSRC_RELEASED, &cp, md);
    return PS_OK;
})

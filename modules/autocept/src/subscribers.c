#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>
#include <lotto/runtime/capture_point.h>

struct autocept_saved_state {
    struct autocept_call_event event;
};

static struct autocept_saved_state _saved_state_key;

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_AUTOCEPT_CALL, {
    struct autocept_saved_state *saved = SELF_TLS(md, &_saved_state_key);
    struct autocept_call_event *ev     = EVENT_PAYLOAD(event);
    capture_point cp                   = {
                          .chain_id = chain,
                          .type_id  = EVENT_AUTOCEPT_CALL,
                          .blocking = true,
                          .payload  = ev,
                          .func     = ev->name,
    };

    saved->event = *ev;
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, EVENT_AUTOCEPT_CALL, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_AUTOCEPT_CALL, {
    struct autocept_saved_state *saved = SELF_TLS(md, &_saved_state_key);
    struct autocept_call_event *ev     = EVENT_PAYLOAD(event);
    struct value ret                   = ev->ret;
    capture_point cp                   = {
                          .chain_id = chain,
                          .type_id  = EVENT_AUTOCEPT_CALL,
                          .blocking = true,
                          .payload  = ev,
                          .func     = ev->name,
    };

    *ev     = saved->event;
    ev->ret = ret;
    cp.func = ev->name;
    PS_PUBLISH(CHAIN_INGRESS_AFTER, EVENT_AUTOCEPT_CALL, &cp, md);
    return PS_OK;
})

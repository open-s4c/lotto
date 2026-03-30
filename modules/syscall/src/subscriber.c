#include "internal.h"
#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>
#include <lotto/modules/syscall/events.h>
#include <lotto/runtime/capture_point.h>

struct syscall_saved_state {
    struct autocept_call_event event;
};

static struct syscall_saved_state _saved_state_key;

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_SYSCALL, {
    struct syscall_saved_state *saved = SELF_TLS(md, &_saved_state_key);
    struct autocept_call_event *ev    = EVENT_PAYLOAD(event);
    capture_point cp                  = {
                         .src_chain = chain,
                         .src_type  = ev->src_type,
                         .blocking  = true,
                         .payload   = ev,
                         .func      = ev->name,
    };

    ASSERT(ev->src_type == EVENT_SYSCALL);
    ev->func     = (void (*)(void))lotto_syscall;
    saved->event = *ev;
    PS_PUBLISH(CHAIN_INGRESS_BEFORE, ev->src_type, &cp, md);
    return PS_STOP_CHAIN;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_SYSCALL, {
    struct syscall_saved_state *saved = SELF_TLS(md, &_saved_state_key);
    struct autocept_call_event *ev    = EVENT_PAYLOAD(event);
    struct value ret                  = ev->ret;
    capture_point cp                  = {
                         .src_chain = chain,
                         .src_type  = ev->src_type,
                         .blocking  = true,
                         .payload   = ev,
                         .func      = ev->name,
    };

    ASSERT(ev->src_type == EVENT_SYSCALL);
    *ev     = saved->event;
    ev->ret = ret;
    cp.func = ev->name;
    PS_PUBLISH(CHAIN_INGRESS_AFTER, ev->src_type, &cp, md);
    return PS_STOP_CHAIN;
})

static bool _ready;
ON_REGISTRATION_PHASE({ _ready = true; })

PS_SUBSCRIBE(INTERCEPT_BEFORE, EVENT_SYSCALL, {
    struct autocept_call_event *ev = EVENT_PAYLOAD(event);
    ASSERT(ev->src_type == EVENT_SYSCALL);
    ev->func = (void (*)(void))lotto_syscall;
    return _ready ? PS_OK : PS_STOP_CHAIN;
})
PS_SUBSCRIBE(INTERCEPT_AFTER, EVENT_SYSCALL, {
    struct autocept_call_event *ev = EVENT_PAYLOAD(event);
    ASSERT(ev->src_type == EVENT_SYSCALL);
    ev->func = (void (*)(void))lotto_syscall;
    return _ready ? PS_OK : PS_STOP_CHAIN;
})

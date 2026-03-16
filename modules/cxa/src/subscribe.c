#include <dice/chains/capture.h>
#include <dice/events/cxa.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_CXA_GUARD_ACQUIRE, {
    struct __cxa_guard_acquire_event *ev = EVENT_PAYLOAD(event);
    context_t *ctx =
        ctx_pc(.pc = (uintptr_t)ev->pc, .cat = CAT_CALL,
               .func = "__cxa_guard_acquire", .args = {arg_ptr(ev->addr)}, );
    intercept_before_call(ctx);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_CXA_GUARD_ACQUIRE, {
    intercept_after_call("__cxa_guard_acquire");
    return PS_OK;
})

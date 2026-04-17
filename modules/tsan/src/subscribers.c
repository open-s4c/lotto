#include <stdbool.h>
#include <stdint.h>

#include <dice/events/memaccess.h>
#include <dice/events/stacktrace.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/logger.h>

static inline uintptr_t
memaccess_event_pc(const void *event)
{
    const void *pc = *(const void *const *)event;
    return (uintptr_t)pc;
}

#define PUBLISH_MEMACCESS(SUFFIX, SRC_TYPE, EV)                                \
    do {                                                                       \
        capture_point cp = {                                                   \
            .payload = (EV),                                                   \
            .pc      = memaccess_event_pc(EV),                                 \
            .func    = (EV)->func,                                             \
        };                                                                     \
        PS_PUBLISH(CHAIN_INGRESS_##SUFFIX, SRC_TYPE, &cp, md);                 \
    } while (0)

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_READ, {
    struct ma_read_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(EVENT, EVENT_MA_READ, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_WRITE, {
    struct ma_write_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(EVENT, EVENT_MA_WRITE, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_AREAD, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_AREAD, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_AWRITE, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_AWRITE, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_RMW, {
    struct ma_rmw_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_RMW, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_RMW, {
    struct ma_rmw_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_RMW, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_XCHG, {
    struct ma_xchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_XCHG, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_XCHG, {
    struct ma_xchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_XCHG, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_CMPXCHG, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_CMPXCHG, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_CMPXCHG, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_CMPXCHG, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_CMPXCHG_WEAK, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_CMPXCHG_WEAK, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_CMPXCHG_WEAK, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_CMPXCHG_WEAK, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_FENCE, {
    struct ma_fence_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(BEFORE, EVENT_MA_FENCE, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_FENCE, {
    struct ma_fence_event *ev = EVENT_PAYLOAD(event);
    PUBLISH_MEMACCESS(AFTER, EVENT_MA_FENCE, ev);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_STACKTRACE_ENTER, {
    stacktrace_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp       = {
              .payload = ev,
              .pc      = (uintptr_t)ev->pc,
              .func    = "func_entry",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, type, &cp, md);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_STACKTRACE_EXIT, {
    stacktrace_event_t *ev = EVENT_PAYLOAD(event);
    capture_point cp       = {
              .payload = ev,
              .pc      = (uintptr_t)ev->pc,
              .func    = "func_exit",
    };
    PS_PUBLISH(CHAIN_INGRESS_EVENT, type, &cp, md);
    return PS_OK;
})

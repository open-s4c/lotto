#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

// -----------------------------------------------------------------------------
// memory accesses
//
// EVENT_MA_READ                 30       ./include/dice/events/memaccess.h
// EVENT_MA_WRITE                31       ./include/dice/events/memaccess.h
// EVENT_MA_AREAD                32       ./include/dice/events/memaccess.h
// EVENT_MA_AWRITE               33       ./include/dice/events/memaccess.h
// EVENT_MA_RMW                  34       ./include/dice/events/memaccess.h
// EVENT_MA_XCHG                 35       ./include/dice/events/memaccess.h
// EVENT_MA_CMPXCHG              36       ./include/dice/events/memaccess.h
// EVENT_MA_CMPXCHG_WEAK         37       ./include/dice/events/memaccess.h
// EVENT_MA_FENCE                38       ./include/dice/events/memaccess.h
// -----------------------------------------------------------------------------
#include <dice/events/memaccess.h>

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_READ, {
    struct ma_read_event *ev = EVENT_PAYLOAD(event);
    context_t *c             = ctx(.cat  = CAT_BEFORE_READ,
                                   .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_WRITE, {
    struct ma_write_event *ev = EVENT_PAYLOAD(event);
    context_t *c              = ctx(.cat  = CAT_BEFORE_WRITE,
                                    .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat  = CAT_BEFORE_AREAD,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat  = CAT_AFTER_AREAD,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat  = CAT_BEFORE_AWRITE,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat  = CAT_AFTER_AWRITE,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

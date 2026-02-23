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

#define EV_PC ((uintptr_t)ev->pc)

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

#define sized_arg(s, uv)                                                       \
    ({                                                                         \
        arg_t _arg;                                                            \
        switch (s) {                                                           \
            case 1:                                                            \
                _arg = (arg_t){.value.u8 = uv.u8, .width = ARG_U8};            \
                break;                                                         \
            case 2:                                                            \
                _arg = (arg_t){.value.u16 = uv.u16, .width = ARG_U16};         \
                break;                                                         \
            case 4:                                                            \
                _arg = (arg_t){.value.u32 = uv.u32, .width = ARG_U32};         \
                break;                                                         \
            case 8:                                                            \
                _arg = (arg_t){.value.u64 = uv.u64, .width = ARG_U64};         \
                break;                                                         \
            default:                                                           \
                ASSERT(0);                                                     \
                break;                                                         \
        };                                                                     \
        _arg;                                                                  \
    })

#define sized_eq(s, uv1, uv2)                                                  \
    ({                                                                         \
        bool _result;                                                          \
        switch (s) {                                                           \
            case 1:                                                            \
                _result = uv1.u8 == uv2.u8;                                    \
                break;                                                         \
            case 2:                                                            \
                _result = uv1.u16 == uv2.u16;                                  \
                break;                                                         \
            case 4:                                                            \
                _result = uv1.u32 == uv2.u32;                                  \
                break;                                                         \
            case 8:                                                            \
                _result = uv1.u64 == uv2.u64;                                  \
                break;                                                         \
            default:                                                           \
                ASSERT(0);                                                     \
                break;                                                         \
        };                                                                     \
        _result;                                                               \
    })

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_READ, {
    struct ma_read_event *ev = EVENT_PAYLOAD(event);
    context_t *c = ctx(.cat = CAT_BEFORE_READ, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_MA_WRITE, {
    struct ma_write_event *ev = EVENT_PAYLOAD(event);
    context_t *c = ctx(.cat = CAT_BEFORE_WRITE, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_AREAD, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AREAD, {
    struct ma_aread_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_AFTER_AREAD, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_AWRITE, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_AWRITE, {
    struct ma_awrite_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_AFTER_AWRITE, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_RMW, {
    struct ma_rmw_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_RMW, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_RMW, {
    struct ma_rmw_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_AFTER_RMW, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_XCHG, {
    struct ma_xchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_XCHG, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_XCHG, {
    struct ma_xchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_AFTER_XCHG, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_CMPXCHG, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_CMPXCHG, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->cmp),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_CMPXCHG, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c =
        ctx(.cat  = sized_eq(ev->size, ev->old, ev->val) ? CAT_AFTER_CMPXCHG_S :
                                                           CAT_AFTER_CMPXCHG_F,
            .func = ev->func, .pc = EV_PC,
            .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                     sized_arg(ev->size, ev->cmp),
                     sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_CMPXCHG_WEAK, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_CMPXCHG, .func = ev->func, .pc = EV_PC,
                       .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                                sized_arg(ev->size, ev->cmp),
                                sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_CMPXCHG_WEAK, {
    struct ma_cmpxchg_event *ev = EVENT_PAYLOAD(event);

    context_t *c =
        ctx(.cat  = sized_eq(ev->size, ev->old, ev->val) ? CAT_AFTER_CMPXCHG_S :
                                                           CAT_AFTER_CMPXCHG_F,
            .func = ev->func, .pc = EV_PC,
            .args = {arg_ptr(ev->addr), arg(size_t, ev->size),
                     sized_arg(ev->size, ev->cmp),
                     sized_arg(ev->size, ev->val)});
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MA_FENCE, {
    struct ma_fence_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_BEFORE_FENCE, .func = ev->func, );
    intercept_capture(c);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_MA_FENCE, {
    struct ma_fence_event *ev = EVENT_PAYLOAD(event);

    context_t *c = ctx(.cat = CAT_AFTER_FENCE, .func = ev->func, );
    intercept_capture(c);
    return PS_OK;
})

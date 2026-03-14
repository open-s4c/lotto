#ifndef LOTTO_PUBSUB_H
#define LOTTO_PUBSUB_H

#include <dice/module.h>
#include <lotto/base/value.h>
#include <lotto/core/events.h>
#include <lotto/core/driver/events.h>
#include <lotto/core/engine/events.h>
#include <lotto/core/runtime/events.h>
#include <lotto/util/macros.h>

#define CHAIN_LOTTO_CONTROL 7
#define CHAIN_LOTTO_DEFAULT 8

#define LOTTO_UNWRAP(...) __VA_ARGS__
#define LOTTO_BODY(...)   __VA_ARGS__

#define LOTTO_SLOT_PASTE_(a, b) a##b
#define LOTTO_SLOT_PASTE(a, b)  LOTTO_SLOT_PASTE_(a, b)
#define LOTTO_SLOT_MAP(k) \
    LOTTO_SLOT_PASTE(DICE_MODULE_SLOT, LOTTO_SLOT_PASTE(00, k))


#define LOTTO_ADVERTISE_TYPE(TYPE)                                             \
    static void __attribute__((constructor(DICE_XTOR_PRIO)))                   \
    lotto_advertise_type_##TYPE##_(void)                                       \
    {                                                                          \
        ps_register_type(TYPE, #TYPE);                                         \
    }

#define LOTTO_SUBSCRIBE_CB_(CHAIN, TYPE, SLOT, ...)                            \
    PS_HANDLER_DEF(CHAIN, TYPE, SLOT, {                                        \
        struct value v = event ? *(struct value *)event : nil;                 \
        (void)v;                                                               \
        __VA_ARGS__                                                            \
    })                                                                         \
    static void DICE_CTOR V_JOIN(V_JOIN(ps_subscribe, SLOT), )(void)           \
    {                                                                          \
        int err =                                                              \
            ps_subscribe(CHAIN, TYPE, PS_HANDLER(CHAIN, TYPE, SLOT), SLOT);    \
        if (err != PS_OK)                                                      \
            log_fatal("could not subscribe %s_%s_%u: %d", ps_chain_str(CHAIN), \
                      ps_type_str(TYPE), SLOT, err);                           \
    }

#define LOTTO_SUBSCRIBE_ONCE(TYPE, ...)                                        \
    PS_SUBSCRIBE(CHAIN_LOTTO_DEFAULT, TYPE, LOTTO_BODY({                       \
                     struct value v = event ? *(struct value *)event : nil;    \
                     (void)v;                                                  \
                     __VA_ARGS__                                               \
                 }))

#define LOTTO_SUBSCRIBE(TYPE, ...)                                             \
    LOTTO_SUBSCRIBE_CB_(CHAIN_LOTTO_DEFAULT, TYPE,                             \
                        LOTTO_SLOT_MAP(__COUNTER__),                           \
                        LOTTO_BODY({__VA_ARGS__}))

#define LOTTO_SUBSCRIBE_CONTROL(TYPE, ...)                                     \
    LOTTO_SUBSCRIBE_CB_(CHAIN_LOTTO_CONTROL, TYPE,                             \
                        LOTTO_SLOT_MAP(__COUNTER__),                           \
                        LOTTO_BODY({__VA_ARGS__}))

#define LOTTO_PUBLISH_CONTROL(evtype)                                          \
    PS_PUBLISH(CHAIN_LOTTO_CONTROL, evtype, 0, 0)

#define LOTTO_PUBLISH(evtype, val)                                             \
    PS_PUBLISH(CHAIN_LOTTO_DEFAULT, evtype, (void *)&(val), 0)

#endif

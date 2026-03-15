#ifndef LOTTO_PUBSUB_H
#define LOTTO_PUBSUB_H

#include <dice/module.h>
#include <lotto/base/value.h>
#include <lotto/core/driver/events.h>
#include <lotto/core/engine/events.h>
#include <lotto/core/events.h>
#include <lotto/core/runtime/events.h>
#include <lotto/util/macros.h>

/* Lotto pubsub chains. */
#define CHAIN_LOTTO_CONTROL 7
#define CHAIN_LOTTO_DEFAULT 8

/* Advertise a Lotto event type name for debugging and tracing output. */
#define LOTTO_ADVERTISE_TYPE(TYPE)                                             \
    static void __attribute__((constructor(DICE_XTOR_PRIO)))                   \
    lotto_advertise_type_##TYPE##_(void)                                       \
    {                                                                          \
        ps_register_type(TYPE, #TYPE);                                         \
    }

/* Subscribe once on the default Lotto chain using Dice's native helper. */
#define LOTTO_SUBSCRIBE_ONCE(TYPE, ...)                                        \
    PS_SUBSCRIBE(CHAIN_LOTTO_DEFAULT, TYPE, LOTTO_BODY({                       \
                     struct value v = event ? *(struct value *)event : nil;    \
                     (void)v;                                                  \
                     __VA_ARGS__                                               \
                 }))

/*
 * Subscribe on the default Lotto chain.
 *
 * The callback receives `v` as the payload converted to `struct value`.
 */
#define LOTTO_SUBSCRIBE(TYPE, ...)                                             \
    LOTTO_SUBSCRIBE_WITH_K_(CHAIN_LOTTO_DEFAULT, TYPE, __COUNTER__,            \
                            LOTTO_BODY({__VA_ARGS__}))

/* Subscribe on the Lotto control chain. */
#define LOTTO_SUBSCRIBE_CONTROL(TYPE, ...)                                     \
    LOTTO_SUBSCRIBE_WITH_K_(CHAIN_LOTTO_CONTROL, TYPE, __COUNTER__,            \
                            LOTTO_BODY({__VA_ARGS__}))

/*
 * Run code during Lotto Phase 2: registration.
 *
 * Use this for categories, flags, states, handlers, and similar startup
 * metadata that must exist before initialization runs.
 */
#define ON_REGISTRATION_PHASE(...)                                             \
    LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_REGISTER, __VA_ARGS__)

/*
 * Run code during Lotto Phase 3: initialization.
 *
 * Use this for work that must happen only after registration is complete.
 */
#define ON_INITIALIZATION_PHASE(...)                                           \
    LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_INIT, __VA_ARGS__)

/*
 * Publish the registration-phase event for the current process.
 *
 * This starts Lotto Phase 2.
 */
#define START_REGISTRATION_PHASE() LOTTO_PUBLISH_CONTROL(EVENT_LOTTO_REGISTER)

/*
 * Publish the initialization-phase event for the current process.
 *
 * This starts Lotto Phase 3.
 */
#define START_INITIALIZATION_PHASE() LOTTO_PUBLISH_CONTROL(EVENT_LOTTO_INIT)

/* Publish a control event on the Lotto control chain. */
#define LOTTO_PUBLISH_CONTROL(evtype)                                          \
    PS_PUBLISH(CHAIN_LOTTO_CONTROL, evtype, 0, 0)

/* Publish a value payload on the default Lotto chain. */
#define LOTTO_PUBLISH(evtype, val)                                             \
    PS_PUBLISH(CHAIN_LOTTO_DEFAULT, evtype, (void *)&(val), 0)

/*
 * Internal macro plumbing.
 *
 * These helpers exist only to implement the public macros above. Keep direct
 * use local to this header.
 */
#define LOTTO_UNWRAP(...) __VA_ARGS__
#define LOTTO_BODY(...)   __VA_ARGS__

#define LOTTO_SLOT_PASTE_(a, b) a##b
#define LOTTO_SLOT_PASTE(a, b)  LOTTO_SLOT_PASTE_(a, b)
#define LOTTO_SLOT_BASE         1000
#define LOTTO_SLOT_VALUE(k)     (((int)LOTTO_SLOT_BASE * DICE_MODULE_SLOT) + (k))
#define LOTTO_SLOT_TAG(k)       V_JOIN(DICE_MODULE_SLOT, k)
#define LOTTO_HANDLER_NAME(TAG) V_JOIN(ps_handler, TAG)

/*
 * Internal helper used by Lotto subscription macros.
 *
 * `SLOT` is the numeric pubsub slot that controls callback order.
 * `TAG` is a token-safe suffix used to build unique C symbol names.
 */
#define LOTTO_SUBSCRIBE_CB_(CHAIN, TYPE, SLOT, TAG, ...)                       \
    static inline enum ps_err LOTTO_HANDLER_NAME(TAG)(                         \
        const chain_id chain, const type_id type, void *event,                 \
        struct metadata *md)                                                   \
    {                                                                          \
        struct value v = event ? *(struct value *)event : nil;                 \
        (void)v;                                                               \
        (void)chain;                                                           \
        (void)type;                                                            \
        (void)md;                                                              \
        __VA_ARGS__                                                            \
        return PS_OK;                                                          \
    }                                                                          \
    static void DICE_CTOR V_JOIN(V_JOIN(ps_subscribe, TAG), )(void)            \
    {                                                                          \
        int err = ps_subscribe(CHAIN, TYPE, LOTTO_HANDLER_NAME(TAG), SLOT);    \
        if (err != PS_OK)                                                      \
            log_fatal("could not subscribe %s_%s_%u: %d", ps_chain_str(CHAIN), \
                      ps_type_str(TYPE), SLOT, err);                           \
    }

#define LOTTO_SUBSCRIBE_WITH_K_(CHAIN, TYPE, K, ...)                           \
    LOTTO_SUBSCRIBE_CB_(CHAIN, TYPE, LOTTO_SLOT_VALUE(K), LOTTO_SLOT_TAG(K),   \
                        __VA_ARGS__)

#endif

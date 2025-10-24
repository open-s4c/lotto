/*
 */
#ifndef LOTTO_PUBSUB_INTERFACE_H
#define LOTTO_PUBSUB_INTERFACE_H

#ifndef LOTTO_PUBSUB_H
    #error "lotto/brokers/pubsub.h" should be included, which includes this header
#endif

#ifdef LOTTO_PUBSUB_INTERFACE

    #ifndef LOCAL_CALLEE
        #error                                                                 \
            "MACRO DEFINITION LOCAL_CALLEE needs to be defined in the source file before including pubsub_interface"
    #endif

    #include <lotto/brokers/pubsub/pubsub_callee_autogen.h>
    #include <lotto/brokers/pubsub/pubsub_internal_autogen.h>
    #include <lotto/sys/real.h>

    #define GEN_CALLEE(callee, topic, val)                                     \
        WEAK void _ps_callee_##callee##_##topic(struct value v, void *__);     \
        void (*f__ps_callee_##callee##_##topic)(struct value, void *) =        \
            _ps_callee_##callee##_##topic;                                     \
        PS_INTERFACE_CALLEE_LOOKUP(callee, topic);                             \
        if (NULL != f__ps_callee_##callee##_##topic)                           \
            f__ps_callee_##callee##_##topic(val, NULL);

    #define GEN_CALLEES(topic, val) GEN(CALLEE, topic, val, CALLEES_##topic)
    #define GEN(X, T, V, ...)       V_NR_VARS(GEN_##X, __VA_ARGS__)(T, V, __VA_ARGS__)
    #define GEN_CALLEE10(T, V, C, ...)                                         \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE9(T, V, __VA_ARGS__)
    #define GEN_CALLEE9(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE8(T, V, __VA_ARGS__)
    #define GEN_CALLEE8(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE7(T, V, __VA_ARGS__)
    #define GEN_CALLEE7(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE6(T, V, __VA_ARGS__)
    #define GEN_CALLEE6(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE5(T, V, __VA_ARGS__)
    #define GEN_CALLEE5(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE4(T, V, __VA_ARGS__)
    #define GEN_CALLEE4(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE3(T, V, __VA_ARGS__)
    #define GEN_CALLEE3(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE2(T, V, __VA_ARGS__)
    #define GEN_CALLEE2(T, V, C, ...)                                          \
        GEN_CALLEE(C, T, V);                                                   \
        GEN_CALLEE1(T, V, __VA_ARGS__)
    #define GEN_CALLEE1(T, V, C) GEN_CALLEE(C, T, V);


    #define _PS_SUBSCRIBE_INTERFACE(callee, topic, CALLBACK)                   \
        PS_SUBSCRIBE_INTERFACE_##topic(callee, topic, CALLBACK)

    // PS Interface generated
    #define PS_SUBSCRIBE_INTERFACE(topic, CALLBACK)                            \
        _PS_SUBSCRIBE_INTERFACE(LOCAL_CALLEE, topic, CALLBACK)

    #define PS_PUBLISH_INTERFACE(topic, val)                                   \
        PS_PUBLISH_INTERFACE_##topic(topic, val)

    #define PS_INTERFACE_CALLEE_LOOKUP(callee, topic)                          \
        {                                                                      \
            if (NULL == f__ps_callee_##callee##_##topic) {                     \
                f__ps_callee_##callee##_##topic = real_func_try(               \
                    REAL_APPLY(STR, f__ps_callee_##callee##_##topic), 0);      \
            }                                                                  \
        }
#else
    // PS interface not generated, fall back to normal pub sub
    #define PS_SUBSCRIBE_INTERFACE(topic, CALLBACK)                            \
        PS_SUBSCRIBE(topic, CALLBACK)

    #define PS_PUBLISH_INTERFACE(topic, val) ps_publish(topic, val);
#endif

#endif

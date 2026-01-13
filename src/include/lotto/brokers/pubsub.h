#ifndef LOTTO_PUBSUB_H
#define LOTTO_PUBSUB_H

#include <lotto/base/topic.h>
#include <lotto/base/value.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/util/macros.h>

typedef void (*lotto_ps_callback_f)(struct value v, void *arg);
void lotto_ps_subscribe(topic_t t, lotto_ps_callback_f cb, void *arg);
void lotto_ps_publish(topic_t t, struct value v);

#define LOTTO_PS_SUBSCRIBE(topic, CALLBACK)                                    \
    static void _lotto_ps_callback_##topic(struct value v, void *__)           \
    {                                                                          \
        (CALLBACK);                                                            \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR _looto_ps_subscribe_##topic(void)            \
    {                                                                          \
        lotto_ps_subscribe(topic, _lotto_ps_callback_##topic, NULL);           \
    }

#endif

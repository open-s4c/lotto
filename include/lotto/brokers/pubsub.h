#ifndef LOTTO_PUBSUB_H
#define LOTTO_PUBSUB_H

#include <lotto/base/topic.h>
#include <lotto/base/value.h>
#include <lotto/brokers/pubsub_interface.h>
#include <lotto/util/macros.h>

typedef void (*ps_callback_f)(struct value v, void *arg);
void lotto_ps_subscribe(topic_t t, ps_callback_f cb, void *arg);
void lotto_ps_publish(topic_t t, struct value v);

#define ps_subscribe lotto_ps_subscribe
#define ps_publish    lotto_ps_publish

#define PS_SUBSCRIBE(topic, CALLBACK)                                          \
    static void _ps_callback_##topic(struct value v, void *__)                 \
    {                                                                          \
        (CALLBACK);                                                            \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR _ps_subscribe_##topic(void)                  \
    {                                                                          \
        ps_subscribe(topic, _ps_callback_##topic, NULL);                       \
    }

#endif

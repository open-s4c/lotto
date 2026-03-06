#ifndef LOTTO_PUBSUB_H
#define LOTTO_PUBSUB_H

#include <lotto/base/topic.h>
#include <lotto/base/value.h>
#include <lotto/util/macros.h>
#include <dice/module.h>

#ifndef CHAIN_LOTTO
    #define CHAIN_LOTTO 7
#endif

#define LOTTO_SUBSCRIBE(topic, CALLBACK)                                \
    PS_SUBSCRIBE(CHAIN_LOTTO, topic, {                                         \
        struct value v = *(struct value *)event;                               \
        (void)v;                                                               \
        CALLBACK                                                               \
    })

#define LOTTO_PUBLISH(topic, val)                                       \
    PS_PUBLISH(CHAIN_LOTTO, topic, (void *)&(val), 0);

#endif

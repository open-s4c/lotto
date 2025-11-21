/*
 */
#include <lotto/brokers/pubsub.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>

#define MAX_SUBSCRIPTIONS 1024

typedef struct {
    topic_t topic;
    ps_callback_f cb;
    void *arg;
} subscription_t;

static int _next_subscription;
static subscription_t _subscriptions[MAX_SUBSCRIPTIONS + 1];


void
ps_subscribe(topic_t t, ps_callback_f cb, void *arg)
{
    ASSERT(_next_subscription < MAX_SUBSCRIPTIONS);
    int idx             = _next_subscription++;
    _subscriptions[idx] = (subscription_t){.topic = t, .cb = cb, .arg = arg};
}

void
ps_publish(topic_t t, struct value v)
{
    for (int idx = 0; idx < _next_subscription; idx++) {
        subscription_t subs = _subscriptions[idx];
        if (subs.topic != t)
            continue;
        ASSERT(subs.cb);
        subs.cb(v, subs.arg);
    }
}

/*
 * Minimal pubsub stub for the trimmed build.
 */
#include <lotto/brokers/pubsub.h>

void
lotto_ps_subscribe(topic_t t, lotto_ps_callback_f cb, void *arg)
{
    (void)t;
    (void)cb;
    (void)arg;
}

void
lotto_ps_publish(topic_t t, struct value v)
{
    (void)t;
    (void)v;
}

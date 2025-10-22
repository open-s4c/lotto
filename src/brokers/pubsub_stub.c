/*
 * Minimal pubsub stub for the trimmed build.
 */
#include <lotto/brokers/pubsub.h>

void
ps_subscribe(topic_t t, ps_callback_f cb, void *arg)
{
    (void)t;
    (void)cb;
    (void)arg;
}

void
ps_publish(topic_t t, struct value v)
{
    (void)t;
    (void)v;
}

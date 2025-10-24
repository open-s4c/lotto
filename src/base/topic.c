/*
 */
#include <lotto/base/topic.h>
#include <lotto/sys/assert.h>

#define GEN_TOPIC(topic) [TOPIC_##topic] = #topic,
static const char *_topic_map[] = {FOR_EACH_TOPIC};
#undef GEN_TOPIC

BIT_STR(topic, TOPIC)

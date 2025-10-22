#ifndef LOTTO_TOPIC_H
#define LOTTO_TOPIC_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/value.h>

#define FOR_EACH_TOPIC                                                         \
    GEN_TOPIC(ENGINE_START)                                                    \
    GEN_TOPIC(REPLAY_END)                                                      \
    GEN_TOPIC(BEFORE_CAPTURE)                                                  \
    GEN_TOPIC(BEFORE_MARSHAL_CONFIG)                                           \
    GEN_TOPIC(BEFORE_MARSHAL_FINAL)                                            \
    GEN_TOPIC(BEFORE_MARSHAL_PERSISTENT)                                       \
    GEN_TOPIC(AFTER_UNMARSHAL_CONFIG)                                          \
    GEN_TOPIC(AFTER_UNMARSHAL_FINAL)                                           \
    GEN_TOPIC(AFTER_UNMARSHAL_PERSISTENT)                                      \
    GEN_TOPIC(INFO_RECORD_LOAD)                                                \
    GEN_TOPIC(INFO_RECORD_SAVE)                                                \
    GEN_TOPIC(DELAYED_PATH)                                                    \
    GEN_TOPIC(DEADLOCK_DETECTED)                                               \
    GEN_TOPIC(ENFORCE_VIOLATED)                                                \
    GEN_TOPIC(NEXT_TASK)                                                       \
    GEN_TOPIC(TRIGGER_TIMEOUT)                                                 \
    GEN_TOPIC(MEMMGR_RUNTIME_INIT)                                             \
    GEN_TOPIC(MEMMGR_USER_INIT)

#define GEN_TOPIC(topic) TOPIC_##topic,
typedef enum topic { TOPIC_NONE = 0, FOR_EACH_TOPIC TOPIC_END_ } topic_t;
#undef GEN_TOPIC

/**
 * Returns a string representation of the topic name.
 */
const char *topic_str(topic_t t);

#endif

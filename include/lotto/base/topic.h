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

#define TOPIC_ENGINE_START               1
#define TOPIC_REPLAY_END                 2
#define TOPIC_BEFORE_CAPTURE             3
#define TOPIC_BEFORE_MARSHAL_CONFIG      4
#define TOPIC_BEFORE_MARSHAL_FINAL       5
#define TOPIC_BEFORE_MARSHAL_PERSISTENT  6
#define TOPIC_AFTER_UNMARSHAL_CONFIG     7
#define TOPIC_AFTER_UNMARSHAL_FINAL      8
#define TOPIC_AFTER_UNMARSHAL_PERSISTENT 9
#define TOPIC_INFO_RECORD_LOAD           10
#define TOPIC_INFO_RECORD_SAVE           11
#define TOPIC_DELAYED_PATH               12
#define TOPIC_DEADLOCK_DETECTED          13
#define TOPIC_ENFORCE_VIOLATED           14
#define TOPIC_NEXT_TASK                  15
#define TOPIC_TRIGGER_TIMEOUT            16
#define TOPIC_MEMMGR_RUNTIME_INIT        17
#define TOPIC_MEMMGR_USER_INIT           18

/**
 * Returns a string representation of the topic name.
 */
const char *topic_str(topic_t t);

#endif

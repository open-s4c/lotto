/**
 * @file events.h
 * @brief Engine declarations for events.
 */
#ifndef LOTTO_CORE_ENGINE_EVENTS_H
#define LOTTO_CORE_ENGINE_EVENTS_H

#include <lotto/base/reason.h>

struct metadata;

/* Lotto startup phase events driven by the engine. */
#define EVENT_LOTTO_REGISTER 140
#define EVENT_LOTTO_INIT     141
#define EVENT_LOTTO_FINI     142

struct lotto_fini_event {
    struct metadata *md;
    reason_t reason;
};

/* Core engine events. */
#define EVENT_ENGINE__START                      110
#define EVENT_ENGINE__REPLAY_END                 111
#define EVENT_ENGINE__BEFORE_CAPTURE             112
#define EVENT_ENGINE__BEFORE_MARSHAL_CONFIG      113
#define EVENT_ENGINE__BEFORE_MARSHAL_FINAL       114
#define EVENT_ENGINE__BEFORE_MARSHAL_PERSISTENT  115
#define EVENT_ENGINE__AFTER_UNMARSHAL_CONFIG     116
#define EVENT_ENGINE__AFTER_UNMARSHAL_FINAL      117
#define EVENT_ENGINE__AFTER_UNMARSHAL_PERSISTENT 118
#define EVENT_ENGINE__INFO_RECORD_LOAD           119
#define EVENT_ENGINE__INFO_RECORD_SAVE           120
#define EVENT_ENGINE__DELAYED_PATH               121
#define EVENT_ENGINE__NEXT_TASK                  122

#endif

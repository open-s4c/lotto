/**
 * @file events.h
 * @brief QEMU profile marker event identifiers.
 */
#ifndef LOTTO_MODULES_QEMU_PROFILE_EVENTS_H
#define LOTTO_MODULES_QEMU_PROFILE_EVENTS_H

#include <stdint.h>

#define EVENT_QEMU_PROFILE_START 186
#define EVENT_QEMU_PROFILE_END   187

typedef struct qemu_profile_event {
    uint64_t id;
} qemu_profile_event_t;

#endif

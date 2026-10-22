/**
 * @file events.h
 * @brief QEMU snapshot semantic event identifiers.
 */
#ifndef LOTTO_MODULES_QEMU_SNAPSHOT_EVENTS_H
#define LOTTO_MODULES_QEMU_SNAPSHOT_EVENTS_H

#include <stdbool.h>

#ifndef DICE_ROLLED
    #define EVENT_QEMU_SNAPSHOT_REQUEST 188
    #define EVENT_QEMU_SNAPSHOT_DONE    189
#endif

struct qemu_snapshot_done_event {
    const char *name;
    bool success;
};

#endif

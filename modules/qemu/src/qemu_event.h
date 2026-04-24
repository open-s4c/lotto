#ifndef LOTTO_QEMU_EVENT_H
#define LOTTO_QEMU_EVENT_H

#include <stdint.h>

typedef struct qlotto_exit_event {
    uint32_t reason;
} qlotto_exit_event_t;

#endif

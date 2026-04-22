#ifndef LOTTO_MODULES_CLOCK_EVENTS_H
#define LOTTO_MODULES_CLOCK_EVENTS_H

#include <stdint.h>

#define EVENT_CLOCK_READ 197

struct lotto_clock_event {
    const void *pc;
    uint64_t ret;
};

#endif

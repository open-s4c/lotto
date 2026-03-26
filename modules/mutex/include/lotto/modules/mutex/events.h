/**
 * @file events.h
 * @brief Mutex semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_MUTEX_EVENTS_H
#define LOTTO_MODULES_MUTEX_EVENTS_H

#include <stdint.h>

#include <dice/types.h>
#include <lotto/runtime/capture_point.h>

struct mutex_acquire_event {
    const void *pc;
    const char *func;
    void *addr;
};

struct mutex_tryacquire_event {
    const void *pc;
    const char *func;
    void *addr;
    int ret;
};

struct mutex_release_event {
    const void *pc;
    const char *func;
    void *addr;
};

#define EVENT_MUTEX_ACQUIRE    142
#define EVENT_MUTEX_TRYACQUIRE 143
#define EVENT_MUTEX_RELEASE    144

static inline uintptr_t
mutex_event_addr(const capture_point *cp)
{
    switch (cp->src_type) {
        case EVENT_MUTEX_ACQUIRE:
            return (uintptr_t)((struct mutex_acquire_event *)cp->payload)->addr;
        case EVENT_MUTEX_TRYACQUIRE:
            return (uintptr_t)((struct mutex_tryacquire_event *)cp->payload)
                ->addr;
        case EVENT_MUTEX_RELEASE:
            return (uintptr_t)((struct mutex_release_event *)cp->payload)->addr;
        default:
            ASSERT(0);
            return 0;
    }
}

#endif

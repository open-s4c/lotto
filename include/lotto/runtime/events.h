/**
 * @file events.h
 * @brief Runtime declarations for events.
 */
#ifndef LOTTO_CORE_RUNTIME_EVENTS_H
#define LOTTO_CORE_RUNTIME_EVENTS_H

#include <stddef.h>
#include <stdint.h>

#include <dice/events/memaccess.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>

#define EVENT_RUNTIME__MEMMGR_INIT 129
#define EVENT_PTHREAD_DETACH       131
#define EVENT_RUNTIME__NOP         132

static inline bool
has_memaccess_addr(const capture_point *cp)
{
    switch (cp->type_id) {
        case EVENT_MA_READ:
        case EVENT_MA_WRITE:
        case EVENT_MA_AREAD:
        case EVENT_MA_AWRITE:
        case EVENT_MA_RMW:
        case EVENT_MA_XCHG:
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return true;
        default:
            return false;
    }
}

static inline uintptr_t
memaccess_addr(const capture_point *cp)
{
    switch (cp->type_id) {
        case EVENT_MA_READ:
            return (uintptr_t)((struct ma_read_event *)cp->payload)->addr;
        case EVENT_MA_WRITE:
            return (uintptr_t)((struct ma_write_event *)cp->payload)->addr;
        case EVENT_MA_AREAD:
            return (uintptr_t)((struct ma_aread_event *)cp->payload)->addr;
        case EVENT_MA_AWRITE:
            return (uintptr_t)((struct ma_awrite_event *)cp->payload)->addr;
        case EVENT_MA_RMW:
            return (uintptr_t)((struct ma_rmw_event *)cp->payload)->addr;
        case EVENT_MA_XCHG:
            return (uintptr_t)((struct ma_xchg_event *)cp->payload)->addr;
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return (uintptr_t)((struct ma_cmpxchg_event *)cp->payload)->addr;
        default:
            ASSERT(0);
            return 0;
    }
}

static inline size_t
memaccess_size(const capture_point *cp)
{
    switch (cp->type_id) {
        case EVENT_MA_READ:
            return ((struct ma_read_event *)cp->payload)->size;
        case EVENT_MA_WRITE:
            return ((struct ma_write_event *)cp->payload)->size;
        case EVENT_MA_AREAD:
            return ((struct ma_aread_event *)cp->payload)->size;
        case EVENT_MA_AWRITE:
            return ((struct ma_awrite_event *)cp->payload)->size;
        case EVENT_MA_RMW:
            return ((struct ma_rmw_event *)cp->payload)->size;
        case EVENT_MA_XCHG:
            return ((struct ma_xchg_event *)cp->payload)->size;
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return ((struct ma_cmpxchg_event *)cp->payload)->size;
        default:
            ASSERT(0);
            return 0;
    }
}

#endif

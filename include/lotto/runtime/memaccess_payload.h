/**
 * @file memaccess_payload.h
 * @brief Accessors for generic memaccess-related capture payloads.
 */
#ifndef LOTTO_RUNTIME_MEMACCESS_PAYLOAD_H
#define LOTTO_RUNTIME_MEMACCESS_PAYLOAD_H

#include <stdint.h>

#include <dice/events/memaccess.h>
#include <lotto/base/arg.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/sys/assert.h>

typedef enum context_memaccess_event {
    CONTEXT_MA_NONE = 0,
    CONTEXT_MA_BEFORE_READ,
    CONTEXT_MA_BEFORE_WRITE,
    CONTEXT_MA_BEFORE_AREAD,
    CONTEXT_MA_BEFORE_AWRITE,
    CONTEXT_MA_BEFORE_RMW,
    CONTEXT_MA_BEFORE_XCHG,
    CONTEXT_MA_BEFORE_CMPXCHG,
    CONTEXT_MA_BEFORE_FENCE,
    CONTEXT_MA_AFTER_AREAD,
    CONTEXT_MA_AFTER_AWRITE,
    CONTEXT_MA_AFTER_RMW,
    CONTEXT_MA_AFTER_XCHG,
    CONTEXT_MA_AFTER_CMPXCHG_S,
    CONTEXT_MA_AFTER_CMPXCHG_F,
    CONTEXT_MA_AFTER_FENCE,
} context_memaccess_event_t;

static inline bool
_context_memaccess_equal(size_t size, uint64_t lhs, uint64_t rhs)
{
    switch (size) {
        case 1:
            return (uint8_t)lhs == (uint8_t)rhs;
        case 2:
            return (uint16_t)lhs == (uint16_t)rhs;
        case 4:
            return (uint32_t)lhs == (uint32_t)rhs;
        case 8:
            return lhs == rhs;
        default:
            ASSERT(0);
            return false;
    }
}

static inline arg_t
context_memaccess_sized_arg(size_t size, uint64_t value)
{
    switch (size) {
        case 1:
            return (arg_t){.value.u8 = (uint8_t)value, .width = ARG_U8};
        case 2:
            return (arg_t){.value.u16 = (uint16_t)value, .width = ARG_U16};
        case 4:
            return (arg_t){.value.u32 = (uint32_t)value, .width = ARG_U32};
        case 8:
            return (arg_t){.value.u64 = value, .width = ARG_U64};
        default:
            ASSERT(0);
            return (arg_t){0};
    }
}

static inline uintptr_t
context_memaccess_addr(const capture_point *cp)
{
    switch (cp->src_type) {
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
context_memaccess_size(const capture_point *cp)
{
    switch (cp->src_type) {
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

static inline arg_t
context_memaccess_value(const capture_point *cp)
{
    switch (cp->src_type) {
        case EVENT_MA_AWRITE:
            return context_memaccess_sized_arg(
                ((struct ma_awrite_event *)cp->payload)->size,
                ((struct ma_awrite_event *)cp->payload)->val.u64);
        case EVENT_MA_RMW:
            return context_memaccess_sized_arg(
                ((struct ma_rmw_event *)cp->payload)->size,
                ((struct ma_rmw_event *)cp->payload)->val.u64);
        case EVENT_MA_XCHG:
            return context_memaccess_sized_arg(
                ((struct ma_xchg_event *)cp->payload)->size,
                ((struct ma_xchg_event *)cp->payload)->val.u64);
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return context_memaccess_sized_arg(
                ((struct ma_cmpxchg_event *)cp->payload)->size,
                ((struct ma_cmpxchg_event *)cp->payload)->val.u64);
        default:
            return (arg_t){0};
    }
}

static inline arg_t
context_memaccess_cmp(const capture_point *cp)
{
    switch (cp->src_type) {
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return context_memaccess_sized_arg(
                ((struct ma_cmpxchg_event *)cp->payload)->size,
                ((struct ma_cmpxchg_event *)cp->payload)->cmp.u64);
        default:
            return (arg_t){0};
    }
}

static inline uint32_t
context_memaccess_rmw_op(const capture_point *cp)
{
    ASSERT(cp->src_type == EVENT_MA_RMW);
    return (uint32_t)((struct ma_rmw_event *)cp->payload)->op;
}

#endif

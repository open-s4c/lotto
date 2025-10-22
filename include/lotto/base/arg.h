#ifndef LOTTO_ARG_H
#define LOTTO_ARG_H
#include <stdint.h>

#include <lotto/base/category.h>
#include <lotto/base/task_id.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/string.h>
#include <lotto/util/casts.h>

/// @brief FFI-safe representation of a u128.
///
/// The alignment of 128 bit integers in LLVM currently depends on
/// the language, thus we need to define our own type to safely share
/// such values across FFI boundaries.
typedef struct arg_u128 {
    // Endianness is native endianness
    uint8_t bytes[sizeof(__uint128_t)];
} arg_u128_t;

/**
 * Stores one argument.
 */
typedef struct arg {
    enum arg_width {
        ARG_EMPTY,
        ARG_U8,
        ARG_U16,
        ARG_U32,
        ARG_U64,
        ARG_U128,
        ARG_PTR
    } width;
    union arg_value {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        arg_u128_t u128;
        uintptr_t ptr;
    } value;
} arg_t;

/*******************************************************************************
 * Argument constructor macros
 ******************************************************************************/

#define arg_ptr(v)                                                             \
    (arg_t)                                                                    \
    {                                                                          \
        .value.ptr = (uintptr_t)(v), .width = ARG_PTR                          \
    }

#define arg(T, v)                                                              \
    ({                                                                         \
        arg_t _arg;                                                            \
        switch (sizeof(T)) {                                                   \
            case sizeof(uint8_t):                                              \
                _arg = (arg_t){.value.u8 = CAST_TYPE(T, v), .width = ARG_U8};  \
                break;                                                         \
            case sizeof(uint16_t):                                             \
                _arg =                                                         \
                    (arg_t){.value.u16 = CAST_TYPE(T, v), .width = ARG_U16};   \
                break;                                                         \
            case sizeof(uint32_t):                                             \
                _arg =                                                         \
                    (arg_t){.value.u32 = CAST_TYPE(T, v), .width = ARG_U32};   \
                break;                                                         \
            case sizeof(uint64_t):                                             \
                _arg =                                                         \
                    (arg_t){.value.u64 = CAST_TYPE(T, v), .width = ARG_U64};   \
                break;                                                         \
            case sizeof(__uint128_t): {                                        \
                __uint128_t local = (__uint128_t)(v);                          \
                sys_memcpy(&_arg.value.u128.bytes, &local,                     \
                           sizeof(__uint128_t));                               \
                _arg.width = ARG_U128;                                         \
            } break;                                                           \
            default:                                                           \
                ASSERT(0);                                                     \
                break;                                                         \
        };                                                                     \
        _arg;                                                                  \
    })

#define arg_equals(arg, val)                                                   \
    ({                                                                         \
        bool match = false;                                                    \
        switch (arg.width) {                                                   \
            case ARG_U8:                                                       \
                match = (arg.value.u8 == CAST_TYPE(uint8_t, val));             \
                break;                                                         \
            case ARG_U16:                                                      \
                match = (arg.value.u16 == CAST_TYPE(uint16_t, val));           \
                break;                                                         \
            case ARG_U32:                                                      \
                match = (arg.value.u32 == CAST_TYPE(uint32_t, val));           \
                break;                                                         \
            case ARG_U64:                                                      \
                match = (arg.value.u64 == CAST_TYPE(uint64_t, val));           \
                break;                                                         \
            case ARG_U128:                                                     \
                __uint128_t local;                                             \
                sys_memcpy(&local, arg.value.u128.bytes);                      \
                match = (local == (__uint128_t)(val));                         \
                break;                                                         \
            case ARG_PTR:                                                      \
                match = (arg.value.ptr == (uintptr_t)(val));                   \
                break;                                                         \
            default:                                                           \
                ASSERT(0);                                                     \
        }                                                                      \
        match;                                                                 \
    })

#define argmatch(p, val)                                                       \
    ({                                                                         \
        bool match = false;                                                    \
        switch ((val).width) {                                                 \
            case ARG_U8:                                                       \
                match = (*(uint8_t *)((p).value.ptr) == (val).value.u8);       \
                break;                                                         \
            case ARG_U16:                                                      \
                match = (*(uint16_t *)((p).value.ptr) == (val).value.u16);     \
                break;                                                         \
            case ARG_U32:                                                      \
                match = (*(uint32_t *)((p).value.ptr) == (val).value.u32);     \
                break;                                                         \
            case ARG_U64:                                                      \
                match = (*(uint64_t *)((p).value.ptr) == (val).value.u64);     \
                break;                                                         \
            case ARG_U128:                                                     \
                match = (*(__uint128_t *)((p).value.ptr) == (val).value.u128); \
                break;                                                         \
            default:                                                           \
                ASSERT(0);                                                     \
        }                                                                      \
        match;                                                                 \
    })

#endif

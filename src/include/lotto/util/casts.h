/*
 */
#ifndef LOTTO_CASTS_H
#define LOTTO_CASTS_H

#include <stdint.h>
#include <string.h>

#include <lotto/sys/assert.h>

// clang-format off
#if defined(__clang__)
    #define TYPE_MINMAX_BEGIN                                                  \
        _Pragma("clang diagnostic push")                                       \
        _Pragma("clang diagnostic ignored \"-Wshift-count-overflow\"")
    #define TYPE_MINMAX_END                                                    \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
    #define TYPE_MINMAX_BEGIN \
        _Pragma("GCC diagnostic push")                                         \
        _Pragma("GCC diagnostic ignored \"-Wshift-count-overflow\"")           \
        _Pragma("GCC diagnostic ignored \"-Woverflow\"")                       \
        _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
    #define TYPE_MINMAX_END                                                    \
        _Pragma("GCC diagnostic pop")
#else
    #warning "Compiler unknown"
    #define TYPE_MINMAX_BEGIN
    #define TYPE_MINMAX_END
#endif

#if defined(__clang__)
    #define ISUNSIGNED_BEGIN                                                   \
        _Pragma("clang diagnostic push")                                       \
        _Pragma("clang diagnostic ignored \"-Wbool-operation\"")
    #define ISUNSIGNED_END                                                     \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
    #define ISUNSIGNED_BEGIN \
        _Pragma("GCC diagnostic push")                                         \
        _Pragma("GCC diagnostic ignored \"-Wbool-operation\"")                 \
        _Pragma("GCC diagnostic ignored \"-Wtype-limits\"")                    \
        _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")                \
        _Pragma("GCC diagnostic ignored \"-Wbool-compare\"")
    #define ISUNSIGNED_END                                                     \
        _Pragma("GCC diagnostic pop")
#else
    #warning "Compiler unknown"
    #define ISUNSIGNED_BEGIN
    #define ISUNSIGNED_END
#endif

#if defined(__clang__)
    #define CAST_BEGIN                                                         \
        _Pragma("clang diagnostic push")                                       \
        _Pragma("clang diagnostic ignored \"-Wsign-compare\"")                 \
        _Pragma("clang diagnostic ignored \"-Wshift-count-overflow\"")         \
        _Pragma("clang diagnostic ignored \"-Wshorten-64-to-32\"")             \
        _Pragma("clang diagnostic ignored \"-Wbool-operation\"")               \
        _Pragma("clang diagnostic ignored \"-Wimplicit-const-int-float-conversion\"")   \
        _Pragma("clang diagnostic ignored \"-Wtautological-constant-out-of-range-compare\"")
    #define CAST_END                                                           \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
    #define CAST_BEGIN \
        _Pragma("GCC diagnostic push")                                         \
        _Pragma("GCC diagnostic ignored \"-Wtype-limits\"")                    \
        _Pragma("GCC diagnostic ignored \"-Wbool-compare\"")                   \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                   \
        _Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"")        \
        _Pragma("GCC diagnostic ignored \"-Wbool-operation\"")                 \
        _Pragma("GCC diagnostic ignored \"-Woverflow\"")                       \
        _Pragma("GCC diagnostic ignored \"-Wshift-count-overflow\"")           \
        _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")            \
        _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
    #define CAST_END                                                           \
        _Pragma("GCC diagnostic pop")
#else
    #warning "Compiler unknown"
    #define CAST_BEGIN
    #define CAST_END
#endif

#define TYPE_MIN_BEGIN TYPE_MINMAX_BEGIN
#define TYPE_MAX_BEGIN TYPE_MINMAX_BEGIN

#define TYPE_MIN_END TYPE_MINMAX_END
#define TYPE_MAX_END TYPE_MINMAX_END

/*******************************************************************************
 * 
 * All of the following macros only work for
 *   bool, _Bool
 * and primitive types based on 
 *   uint<8,16,32,64>_t,
 *   int<8,16,32,64>_t,
 *
 ******************************************************************************/

// NOLINTBEGIN(bugprone-sizeof-expression)

 #define ISUNSIGNED_TYPE(T)                                                    \
 ({                                                                            \
     ISUNSIGNED_BEGIN                                                          \
     T a = 1;                                                                  \
     (a) >= 0 && ((a = ~(a)) >= 0 ? (a = ~(a), 1) : (a = ~(a), 0));            \
     ISUNSIGNED_END                                                            \
 })

// double typeof() removes const qualifier
#define ISUNSIGNED_VAR(V) ISUNSIGNED_TYPE( typeof((typeof((V)))0) )

#define TYPE_MAX(T)                                                            \
    ({                                                                         \
        TYPE_MAX_BEGIN                                                         \
        (strcmp(#T, "bool") == 0 || strcmp(#T, "_Bool") == 0) ? 1 :            \
        ISUNSIGNED_TYPE(T) ? (uint64_t)(T)(~((T)0)) :                          \
        (int64_t)(((int64_t)1 << ((sizeof(T) * 8) - 1)) - 1);                  \
        TYPE_MAX_END                                                           \
    })

#define TYPE_MIN(T)                                                            \
    ({                                                                         \
        TYPE_MIN_BEGIN                                                         \
        (strcmp(#T, "bool") == 0 || strcmp(#T, "_Bool") == 0) ? 0 :            \
        ISUNSIGNED_TYPE(T) ? (uint64_t) 0 :                                    \
        (int64_t)(((int64_t)1 << (((sizeof(T) * 8)) -1)) * -1);                \
        TYPE_MIN_END                                                           \
    })

#define CAST_TYPE(T, X)                                                        \
    ({                                                                         \
        CAST_BEGIN                                                             \
        ASSERT(ISUNSIGNED_TYPE(T) ?                                            \
             ((X) >= (int64_t)0 && (X) <= (uint64_t)TYPE_MAX(T)) :             \
             sizeof((X)) == 8 && ISUNSIGNED_VAR(X) ?                           \
                (uint64_t)(X) <= (uint64_t)TYPE_MAX(T) :                       \
                ((int64_t)TYPE_MIN(T) <= (int64_t)(X) &&                       \
                (int64_t)TYPE_MAX(T) >= (int64_t)(X)));                        \
        (T)(X);                                                                \
        CAST_END                                                               \
    })

#define CAST_TYPE_FAIL(T, X)                                                   \
    ({                                                                         \
        CAST_BEGIN                                                             \
        ASSERT(!(ISUNSIGNED_TYPE(T) ?                                          \
             ((X) >= (int64_t)0 && (X) <= (uint64_t)TYPE_MAX(T)) :             \
             sizeof((X)) == 8 && ISUNSIGNED_VAR(X) ?                           \
                (uint64_t)(X) <= (uint64_t)TYPE_MAX(T) :                       \
                ((int64_t)TYPE_MIN(T) <= (int64_t)(X) &&                       \
                (int64_t)TYPE_MAX(T) >= (int64_t)(X))));                       \
        (T)(X);                                                                \
        CAST_END                                                               \
    })

// NOLINTEND(bugprone-sizeof-expression)

// clang-format on

#endif

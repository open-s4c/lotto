/*
 */
#include <dlfcn.h>
#ifndef DISABLE_EXECINFO
    #include <execinfo.h>
    #include <link.h>
#endif
#include <stdint.h>
#include <stdio.h>

#ifndef TSAN_INTERFACE_ATOMIC_H
// Part of ABI, do not change.
// https://github.com/llvm/llvm-project/blob/main/libcxx/include/atomic
typedef enum {
    __tsan_memory_order_relaxed,
    __tsan_memory_order_consume,
    __tsan_memory_order_acquire,
    __tsan_memory_order_release,
    __tsan_memory_order_acq_rel,
    __tsan_memory_order_seq_cst
} __tsan_memory_order;

#endif

#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <vsync/common/macros.h>

#define DISABLE_TSAN
#ifdef DISABLE_TSAN
    #define CALL_TSAN(F, ...)
    #define CALL_TSAN_A(F, ...) __lotto_##F(__VA_ARGS__)

    #define TSAN_DECL(T, F, ...) T __tsan_##F(__VA_ARGS__)
#else
    #define CALL_TSAN(F, ...)   REAL(__tsan_##F, __VA_ARGS__)
    #define CALL_TSAN_A(F, ...) CALL_TSAN(F, __VA_ARGS__)

    #define TSAN_DECL(T, F, ...)                                               \
        REAL_DECL(T, __tsan_##F, __VA_ARGS__);                                 \
        REAL_ON_INIT(__tsan_##F);                                              \
        T __tsan_##F(__VA_ARGS__)
#endif

#define CAT_BEFORE(C) V_JOIN(CAT_BEFORE, C)
#define CAT_AFTER(C)  V_JOIN(CAT_AFTER, C)

#define TSAN_VOID_ONE(F, C, SIZE)                                              \
    TSAN_DECL(void, F, void *a)                                                \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = #F, .args = {arg_ptr(a), arg(size_t, SIZE)});          \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE(C)));                        \
        CALL_TSAN(F, a);                                                       \
    }
TSAN_VOID_ONE(read1, READ, 1)
TSAN_VOID_ONE(read2, READ, 2)
TSAN_VOID_ONE(read4, READ, 4)
TSAN_VOID_ONE(read8, READ, 8)
TSAN_VOID_ONE(read16, READ, 16)

TSAN_VOID_ONE(write1, WRITE, 1)
TSAN_VOID_ONE(write2, WRITE, 2)
TSAN_VOID_ONE(write4, WRITE, 4)
TSAN_VOID_ONE(write8, WRITE, 8)
TSAN_VOID_ONE(write16, WRITE, 16)

TSAN_VOID_ONE(unaligned_read2, READ, 2)
TSAN_VOID_ONE(unaligned_read4, READ, 4)
TSAN_VOID_ONE(unaligned_read8, READ, 8)
TSAN_VOID_ONE(unaligned_read16, READ, 16)

TSAN_VOID_ONE(unaligned_write2, WRITE, 2)
TSAN_VOID_ONE(unaligned_write4, WRITE, 4)
TSAN_VOID_ONE(unaligned_write8, WRITE, 8)
TSAN_VOID_ONE(unaligned_write16, WRITE, 16)
#undef TSAN_VOID_ONE

#define TSAN_VOID_TWO(F, C, SIZE)                                              \
    TSAN_DECL(void, F, void *a, void *b)                                       \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = #F, .args = {arg_ptr(a), arg(size_t, SIZE)});          \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE(C)));                        \
        CALL_TSAN(F, a, b);                                                    \
    }

TSAN_VOID_TWO(read1_pc, READ, 1)
TSAN_VOID_TWO(read2_pc, READ, 2)
TSAN_VOID_TWO(read4_pc, READ, 4)
TSAN_VOID_TWO(read8_pc, READ, 8)
TSAN_VOID_TWO(read16_pc, READ, 16)

TSAN_VOID_TWO(write1_pc, WRITE, 1)
TSAN_VOID_TWO(write2_pc, WRITE, 2)
TSAN_VOID_TWO(write4_pc, WRITE, 4)
TSAN_VOID_TWO(write8_pc, WRITE, 8)
TSAN_VOID_TWO(write16_pc, WRITE, 16)
#undef TSAN_VOID_TWO

TSAN_DECL(void, func_entry, void *call_pc)
{
#ifndef DISABLE_EXECINFO
    void *buffer[2];
    int nptrs;
    nptrs = backtrace(buffer, 2);
    ASSERT(nptrs == 2);
    Dl_info info;
    struct link_map *map;
    dladdr1(buffer[1], &info, (void **)&map, RTLD_DL_LINKMAP);
    ASSERT(info.dli_saddr || info.dli_fbase);
    context_t *ctx = ctx(.func = "func_entry", .cat = CAT_FUNC_ENTRY,
                         .args = {arg_ptr(call_pc), arg_ptr(info.dli_sname),
                                  arg_ptr(info.dli_fname),
                                  arg_ptr(buffer[1] - map->l_addr)});
#else
    context_t *ctx = ctx(.func = "func_entry", .cat = CAT_FUNC_ENTRY,
                         .args = {arg_ptr(call_pc)});
#endif
    intercept_capture(ctx);
    CALL_TSAN(func_entry, call_pc);
}

TSAN_DECL(void, func_exit)
{
    context_t *ctx = ctx(.func = "func_exit", .cat = CAT_FUNC_EXIT);
    intercept_capture(ctx);
    CALL_TSAN(func_exit);
}

// NOLINTBEGIN(bugprone-macro-parentheses): T is used in the function signature
#define TSAN_DECL_LOAD(T, F, SIZE)                                             \
    static inline T __lotto_##F(const volatile T *a)                           \
    {                                                                          \
        return __atomic_load_n(a, __ATOMIC_SEQ_CST);                           \
    }                                                                          \
    TSAN_DECL(T, F, const volatile T *a, __tsan_memory_order mo)               \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = #F, .args = {arg_ptr(a), arg(size_t, SIZE)});          \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_AREAD));                     \
        T r = CALL_TSAN_A(F, a);                                               \
        intercept_capture(ctx_cat(ctx, CAT_AFTER_AREAD));                      \
        return r;                                                              \
    }
TSAN_DECL_LOAD(uint8_t, atomic8_load, 1)
TSAN_DECL_LOAD(uint16_t, atomic16_load, 2)
TSAN_DECL_LOAD(uint32_t, atomic32_load, 4)
TSAN_DECL_LOAD(uint64_t, atomic64_load, 8)
// TSAN_DECL_LOAD(__uint128_t, atomic128_load, 16)
#undef TSAN_DECL_LOAD

#define TSAN_DECL_STORE(T, F, SIZE)                                            \
    static inline void __lotto_##F(volatile T *a, T v)                         \
    {                                                                          \
        __atomic_store_n((volatile T *)a, v, __ATOMIC_SEQ_CST);                \
    }                                                                          \
    TSAN_DECL(void, F, volatile T *a, T v, __tsan_memory_order mo)             \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = #F,                                                    \
                .args = {arg_ptr(a), arg(size_t, SIZE), arg(T, v)});           \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_AWRITE));                    \
        CALL_TSAN_A(F, a, v);                                                  \
        intercept_capture(ctx_cat(ctx, CAT_AFTER_AWRITE));                     \
    }
TSAN_DECL_STORE(uint8_t, atomic8_store, 1)
TSAN_DECL_STORE(uint16_t, atomic16_store, 2)
TSAN_DECL_STORE(uint32_t, atomic32_store, 4)
TSAN_DECL_STORE(uint64_t, atomic64_store, 8)
// TSAN_DECL_STORE(__uint128_t, atomic128_store, 16)
#undef TSAN_DECL_STORE

#define LOTTO_CAT_FUNC(A, F) __lotto_##A##_##F
#define DEF_LOTTO_RMW(T, A)                                                    \
    static inline T LOTTO_CAT_FUNC(A, exchange)(volatile T * a, T v)           \
    {                                                                          \
        return __atomic_exchange_n((volatile T *)a, v, __ATOMIC_SEQ_CST);      \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_add)(volatile T * a, T v)          \
    {                                                                          \
        return __atomic_fetch_add((volatile T *)a, v, __ATOMIC_SEQ_CST);       \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_sub)(volatile T * a, T v)          \
    {                                                                          \
        return __atomic_fetch_sub((volatile T *)a, v, __ATOMIC_SEQ_CST);       \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_and)(volatile T * a, T v)          \
    {                                                                          \
        return __atomic_fetch_and((volatile T *)a, v, __ATOMIC_SEQ_CST);       \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_or)(volatile T * a, T v)           \
    {                                                                          \
        return __atomic_fetch_or((volatile T *)a, v, __ATOMIC_SEQ_CST);        \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_xor)(volatile T * a, T v)          \
    {                                                                          \
        return __atomic_fetch_xor((volatile T *)a, v, __ATOMIC_SEQ_CST);       \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_nand)(volatile T * a, T v)         \
    {                                                                          \
        return __atomic_fetch_nand((volatile T *)a, v, __ATOMIC_SEQ_CST);      \
    }

DEF_LOTTO_RMW(uint8_t, atomic8)
DEF_LOTTO_RMW(uint16_t, atomic16)
DEF_LOTTO_RMW(uint32_t, atomic32)
DEF_LOTTO_RMW(uint64_t, atomic64)
// DEF_LOTTO_RMW(__uint128_t, atomic128)
#undef LOTTO_CAT_FUNC
#undef DEF_LOTTO_RMW
#define TSAN_DECL_EXCHANGE_(T, F, SIZE)                                        \
    TSAN_DECL(T, F, volatile T *a, T v, __tsan_memory_order mo)                \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = __FUNCTION__,                                          \
                .args = {arg_ptr(a), arg(size_t, SIZE), arg(T, v)});           \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_XCHG));                      \
        T r = CALL_TSAN_A(F, a, v);                                            \
        intercept_capture(ctx_cat(ctx, CAT_AFTER_XCHG));                       \
        return r;                                                              \
    }

#define TSAN_DECL_EXCHANGE(P, S, F, SIZE)                                      \
    TSAN_DECL_EXCHANGE_(P##S##_t, atomic##S##_##F, SIZE)

TSAN_DECL_EXCHANGE(uint, 8, exchange, 1)
TSAN_DECL_EXCHANGE(uint, 16, exchange, 2)
TSAN_DECL_EXCHANGE(uint, 32, exchange, 4)
TSAN_DECL_EXCHANGE(uint, 64, exchange, 8)
// TSAN_DECL_EXCHANGE(__uint, 128, exchange, 16)
#undef TSAN_DECL_EXCHANGE
#undef TSAN_DECL_EXCHANGE_

#define TSAN_DECL_RMW_(T, F, SIZE, OP)                                         \
    TSAN_DECL(T, F, volatile T *a, T v, __tsan_memory_order mo)                \
    {                                                                          \
        context_t *ctx =                                                       \
            ctx(.func = __FUNCTION__,                                          \
                .args = {arg_ptr(a), arg(size_t, SIZE), arg(T, v),             \
                    arg(int, OP)});                                            \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_RMW));                       \
        T r = CALL_TSAN_A(F, a, v);                                            \
        intercept_capture(ctx_cat(ctx, CAT_AFTER_RMW));                        \
        return r;                                                              \
    }

#define TSAN_DECL_RMW(P, S, F, SIZE, OP)                                       \
    TSAN_DECL_RMW_(P##S##_t, atomic##S##_##F, SIZE, OP)

TSAN_DECL_RMW(uint, 8, fetch_add, 1, RMW_OP_ADD)
TSAN_DECL_RMW(uint, 16, fetch_add, 2, RMW_OP_ADD)
TSAN_DECL_RMW(uint, 32, fetch_add, 4, RMW_OP_ADD)
TSAN_DECL_RMW(uint, 64, fetch_add, 8, RMW_OP_ADD)
// TSAN_DECL_RMW(__uint, 128, fetch_add, 16, RMW_OP_ADD)

TSAN_DECL_RMW(uint, 8, fetch_sub, 1, RMW_OP_SUB)
TSAN_DECL_RMW(uint, 16, fetch_sub, 2, RMW_OP_SUB)
TSAN_DECL_RMW(uint, 32, fetch_sub, 4, RMW_OP_SUB)
TSAN_DECL_RMW(uint, 64, fetch_sub, 8, RMW_OP_SUB)
// TSAN_DECL_RMW(__uint, 128, fetch_sub, 16, RMW_OP_SUB)

TSAN_DECL_RMW(uint, 8, fetch_and, 1, RMW_OP_AND)
TSAN_DECL_RMW(uint, 16, fetch_and, 2, RMW_OP_AND)
TSAN_DECL_RMW(uint, 32, fetch_and, 4, RMW_OP_AND)
TSAN_DECL_RMW(uint, 64, fetch_and, 8, RMW_OP_AND)
// TSAN_DECL_RMW(__uint, 128, fetch_and, 16, RMW_OP_AND)

TSAN_DECL_RMW(uint, 8, fetch_or, 1, RMW_OP_OR)
TSAN_DECL_RMW(uint, 16, fetch_or, 2, RMW_OP_OR)
TSAN_DECL_RMW(uint, 32, fetch_or, 4, RMW_OP_OR)
TSAN_DECL_RMW(uint, 64, fetch_or, 8, RMW_OP_OR)
// TSAN_DECL_RMW(__uint, 128, fetch_or, 16, RMW_OP_OR)

TSAN_DECL_RMW(uint, 8, fetch_xor, 1, RMW_OP_XOR)
TSAN_DECL_RMW(uint, 16, fetch_xor, 2, RMW_OP_XOR)
TSAN_DECL_RMW(uint, 32, fetch_xor, 4, RMW_OP_XOR)
TSAN_DECL_RMW(uint, 64, fetch_xor, 8, RMW_OP_XOR)
// TSAN_DECL_RMW(__uint, 128, fetch_xor, 16, RMW_OP_XOR)

TSAN_DECL_RMW(uint, 8, fetch_nand, 1, RMW_OP_NAND)
TSAN_DECL_RMW(uint, 16, fetch_nand, 2, RMW_OP_NAND)
TSAN_DECL_RMW(uint, 32, fetch_nand, 4, RMW_OP_NAND)
TSAN_DECL_RMW(uint, 64, fetch_nand, 8, RMW_OP_NAND)
// TSAN_DECL_RMW(__uint, 128, fetch_nand, 16, RMW_OP_NAND)
#undef TSAN_DECL_RMW
#undef TSAN_DECL_RMW_

#define TSAN_DECL_CMPXCHG(T, F, SIZE)                                          \
    int __lotto_##F##_compare_exchange_strong(volatile T *a, T *c, T v)        \
    {                                                                          \
        return __atomic_compare_exchange_n(                                    \
            (volatile T *)a, c, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);     \
    }                                                                          \
    TSAN_DECL(int, F##_compare_exchange_strong, volatile T *a, T *c, T v,      \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        context_t *ctx = ctx(.func = #F "_compare_exchange_strong",            \
                             .args = {arg_ptr(a), arg(size_t, SIZE),           \
                                      arg(T, *c), arg(T, v)});                 \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_CMPXCHG));                   \
        int r = CALL_TSAN_A(F##_compare_exchange_strong, a, c, v);             \
        intercept_capture(ctx_cat(                                             \
            ctx, ((*a == v) ? CAT_AFTER_CMPXCHG_S : CAT_AFTER_CMPXCHG_F)));    \
        return r;                                                              \
    }                                                                          \
    int __lotto_##F##_compare_exchange_weak(volatile T *a, T *c, T v)          \
    {                                                                          \
        return __lotto_##F##_compare_exchange_strong(a, c, v);                 \
    }                                                                          \
    TSAN_DECL(int, F##_compare_exchange_weak, volatile T *a, T *c, T v,        \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        context_t *ctx = ctx(.func = #F "_compare_exchange_weak",              \
                             .args = {arg_ptr(a), arg(size_t, SIZE),           \
                                      arg(T, *c), arg(T, v)});                 \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_CMPXCHG));                   \
        int r = CALL_TSAN_A(F##_compare_exchange_weak, a, c, v);               \
        intercept_capture(ctx_cat(                                             \
            ctx, ((*a == v) ? CAT_AFTER_CMPXCHG_S : CAT_AFTER_CMPXCHG_F)));    \
        return r;                                                              \
    }                                                                          \
    T __lotto_##F##_compare_exchange_val(volatile T *a, T c, T v)              \
    {                                                                          \
        (void)__lotto_##F##_compare_exchange_strong(a, &c, v);                 \
        return c;                                                              \
    }                                                                          \
    TSAN_DECL(T, F##_compare_exchange_val, volatile T *a, T c, T v,            \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        context_t *ctx = ctx(.func = #F "_compare_exchange_val",               \
                             .args = {arg_ptr(a), arg(size_t, SIZE),           \
                                      arg(T, c), arg(T, v)});                  \
        intercept_capture(ctx_cat(ctx, CAT_BEFORE_CMPXCHG));                   \
        T r = CALL_TSAN_A(F##_compare_exchange_val, a, c, v);                  \
        intercept_capture(ctx_cat(                                             \
            ctx, ((*a == v) ? CAT_AFTER_CMPXCHG_S : CAT_AFTER_CMPXCHG_F)));    \
        return r;                                                              \
    }

TSAN_DECL_CMPXCHG(uint8_t, atomic8, 1)
TSAN_DECL_CMPXCHG(uint16_t, atomic16, 2)
TSAN_DECL_CMPXCHG(uint32_t, atomic32, 4)
TSAN_DECL_CMPXCHG(uint64_t, atomic64, 8)
// TSAN_DECL_CMPXCHG(__uint128_t, atomic128, 16)
#undef TSAN_DECL_CMPXCHG
// NOLINTEND(bugprone-macro-parentheses)

TSAN_DECL(void, atomic_thread_fence, __tsan_memory_order mo)
{
    context_t *ctx = ctx(.func = "atomic_thread_fence");
    intercept_capture(ctx_cat(ctx, CAT_BEFORE_FENCE));
    CALL_TSAN(atomic_thread_fence, __ATOMIC_SEQ_CST);
    intercept_capture(ctx_cat(ctx, CAT_AFTER_FENCE));
}

TSAN_DECL(void, atomic_signal_fence, __tsan_memory_order mo)
{
    context_t *ctx = ctx(.func = "atomic_signal_fence");
    intercept_capture(ctx_cat(ctx, CAT_BEFORE_FENCE));
    CALL_TSAN(atomic_signal_fence, __ATOMIC_SEQ_CST);
    intercept_capture(ctx_cat(ctx, CAT_AFTER_FENCE));
}
#undef TSAN_DECL
#undef CALL_TSAN
#undef CALL_TSAN_A

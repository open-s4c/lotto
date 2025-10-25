#include <stdint.h>
#include <stddef.h>

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>

typedef enum {
    __tsan_memory_order_relaxed,
    __tsan_memory_order_consume,
    __tsan_memory_order_acquire,
    __tsan_memory_order_release,
    __tsan_memory_order_acq_rel,
    __tsan_memory_order_seq_cst
} __tsan_memory_order;

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

#ifndef V_JOIN
    #define V_JOIN(a, b) a##_##b
    #define LOTTO_INTERCEPT_TSAN_DEFINED_V_JOIN 1
#endif
#define CAT_BEFORE(c) V_JOIN(CAT_BEFORE, c)
#define CAT_AFTER(c)  V_JOIN(CAT_AFTER, c)

static inline void
capture_simple(category_t cat, const char *func, arg_t arg0, arg_t arg1)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = func;
        c->cat       = cat;
        c->args[0]   = arg0;
        c->args[1]   = arg1;
        intercept_capture(c);
    }
}

#define TSAN_VOID_ONE(F, C, SIZE)                                              \
    TSAN_DECL(void, F, void *a)                                                \
    {                                                                          \
        capture_simple(CAT_BEFORE(C), #F, arg_ptr(a), arg(size_t, SIZE));      \
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
        capture_simple(CAT_BEFORE(C), #F, arg_ptr(a), arg(size_t, SIZE));      \
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
    capture_simple(CAT_FUNC_ENTRY, "func_entry", arg_ptr(call_pc), arg_ptr(NULL));
    CALL_TSAN(func_entry, call_pc);
}

TSAN_DECL(void, func_exit)
{
    capture_simple(CAT_FUNC_EXIT, "func_exit", arg_ptr(NULL), arg_ptr(NULL));
    CALL_TSAN(func_exit);
}

#define TSAN_DECL_LOAD(T, F, SIZE)                                             \
    static inline T __lotto_##F(const volatile T *a)                           \
    {                                                                          \
        return __atomic_load_n(a, __ATOMIC_SEQ_CST);                           \
    }                                                                          \
    TSAN_DECL(T, F, const volatile T *a, __tsan_memory_order mo)               \
    {                                                                          \
        capture_simple(CAT_BEFORE_AREAD, #F, arg_ptr(a), arg(size_t, SIZE));   \
        T r = CALL_TSAN_A(F, a);                                               \
        capture_simple(CAT_AFTER_AREAD, #F, arg_ptr(a), arg(size_t, SIZE));    \
        return r;                                                              \
    }

TSAN_DECL_LOAD(uint8_t, atomic8_load, 1)
TSAN_DECL_LOAD(uint16_t, atomic16_load, 2)
TSAN_DECL_LOAD(uint32_t, atomic32_load, 4)
TSAN_DECL_LOAD(uint64_t, atomic64_load, 8)
#undef TSAN_DECL_LOAD

#define TSAN_DECL_STORE(T, F, SIZE)                                            \
    static inline void __lotto_##F(volatile T *a, T v)                         \
    {                                                                          \
        __atomic_store_n(a, v, __ATOMIC_SEQ_CST);                              \
    }                                                                          \
    TSAN_DECL(void, F, volatile T *a, T v, __tsan_memory_order mo)             \
    {                                                                          \
        capture_simple(CAT_BEFORE_AWRITE, #F,                                  \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        CALL_TSAN_A(F, a, v);                                                  \
        capture_simple(CAT_AFTER_AWRITE, #F,                                   \
                       arg_ptr(a), arg(size_t, SIZE));                         \
    }

TSAN_DECL_STORE(uint8_t, atomic8_store, 1)
TSAN_DECL_STORE(uint16_t, atomic16_store, 2)
TSAN_DECL_STORE(uint32_t, atomic32_store, 4)
TSAN_DECL_STORE(uint64_t, atomic64_store, 8)
#undef TSAN_DECL_STORE

#define LOTTO_CAT_FUNC(A, F) __lotto_##A##_##F
#define DEF_LOTTO_RMW(T, A)                                                    \
    static inline T LOTTO_CAT_FUNC(A, exchange)(volatile T *a, T v)            \
    {                                                                          \
        return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);                    \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_add)(volatile T *a, T v)           \
    {                                                                          \
        return __atomic_fetch_add(a, v, __ATOMIC_SEQ_CST);                     \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_sub)(volatile T *a, T v)           \
    {                                                                          \
        return __atomic_fetch_sub(a, v, __ATOMIC_SEQ_CST);                     \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_and)(volatile T *a, T v)           \
    {                                                                          \
        return __atomic_fetch_and(a, v, __ATOMIC_SEQ_CST);                     \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_or)(volatile T *a, T v)            \
    {                                                                          \
        return __atomic_fetch_or(a, v, __ATOMIC_SEQ_CST);                      \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_xor)(volatile T *a, T v)           \
    {                                                                          \
        return __atomic_fetch_xor(a, v, __ATOMIC_SEQ_CST);                     \
    }                                                                          \
    static inline T LOTTO_CAT_FUNC(A, fetch_nand)(volatile T *a, T v)          \
    {                                                                          \
        return __atomic_fetch_nand(a, v, __ATOMIC_SEQ_CST);                    \
    }

DEF_LOTTO_RMW(uint8_t, atomic8)
DEF_LOTTO_RMW(uint16_t, atomic16)
DEF_LOTTO_RMW(uint32_t, atomic32)
DEF_LOTTO_RMW(uint64_t, atomic64)
#undef LOTTO_CAT_FUNC
#undef DEF_LOTTO_RMW

#define TSAN_DECL_EXCHANGE(T, NAME, SIZE)                                      \
    TSAN_DECL(T, NAME, volatile T *a, T v, __tsan_memory_order mo)             \
    {                                                                          \
        capture_simple(CAT_BEFORE_XCHG, #NAME,                                 \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        T r = CALL_TSAN_A(NAME, a, v);                                         \
        capture_simple(CAT_AFTER_XCHG, #NAME,                                  \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        return r;                                                              \
    }

TSAN_DECL_EXCHANGE(uint8_t, atomic8_exchange, 1)
TSAN_DECL_EXCHANGE(uint16_t, atomic16_exchange, 2)
TSAN_DECL_EXCHANGE(uint32_t, atomic32_exchange, 4)
TSAN_DECL_EXCHANGE(uint64_t, atomic64_exchange, 8)
#undef TSAN_DECL_EXCHANGE

#define TSAN_DECL_RMW(T, NAME, SIZE)                                           \
    TSAN_DECL(T, NAME, volatile T *a, T v, __tsan_memory_order mo)             \
    {                                                                          \
        capture_simple(CAT_BEFORE_RMW, #NAME,                                  \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        T r = CALL_TSAN_A(NAME, a, v);                                         \
        capture_simple(CAT_AFTER_RMW, #NAME,                                   \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        return r;                                                              \
    }

TSAN_DECL_RMW(uint8_t, atomic8_fetch_add, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_add, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_add, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_add, 8)
TSAN_DECL_RMW(uint8_t, atomic8_fetch_sub, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_sub, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_sub, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_sub, 8)
TSAN_DECL_RMW(uint8_t, atomic8_fetch_and, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_and, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_and, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_and, 8)
TSAN_DECL_RMW(uint8_t, atomic8_fetch_or, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_or, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_or, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_or, 8)
TSAN_DECL_RMW(uint8_t, atomic8_fetch_xor, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_xor, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_xor, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_xor, 8)
TSAN_DECL_RMW(uint8_t, atomic8_fetch_nand, 1)
TSAN_DECL_RMW(uint16_t, atomic16_fetch_nand, 2)
TSAN_DECL_RMW(uint32_t, atomic32_fetch_nand, 4)
TSAN_DECL_RMW(uint64_t, atomic64_fetch_nand, 8)
#undef TSAN_DECL_RMW

#define TSAN_DECL_CMPXCHG(T, NAME, SIZE)                                       \
    static inline int __lotto_##NAME##_compare_exchange_strong(volatile T *a,  \
                                                               T *c, T v)      \
    {                                                                          \
        return __atomic_compare_exchange_n(a, c, v, 0, __ATOMIC_SEQ_CST,       \
                                           __ATOMIC_SEQ_CST);                  \
    }                                                                          \
    TSAN_DECL(int, NAME##_compare_exchange_strong, volatile T *a, T *c, T v,   \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        capture_simple(CAT_BEFORE_CMPXCHG, #NAME "_compare_exchange_strong",   \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        int r = CALL_TSAN_A(NAME##_compare_exchange_strong, a, c, v);          \
        capture_simple((r ? CAT_AFTER_CMPXCHG_S : CAT_AFTER_CMPXCHG_F),        \
                       #NAME "_compare_exchange_strong", arg_ptr(a),           \
                       arg(size_t, SIZE));                                     \
        return r;                                                              \
    }                                                                          \
    static inline int __lotto_##NAME##_compare_exchange_weak(volatile T *a,    \
                                                             T *c, T v)        \
    {                                                                          \
        return __lotto_##NAME##_compare_exchange_strong(a, c, v);              \
    }                                                                          \
    TSAN_DECL(int, NAME##_compare_exchange_weak, volatile T *a, T *c, T v,     \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        capture_simple(CAT_BEFORE_CMPXCHG, #NAME "_compare_exchange_weak",     \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        int r = CALL_TSAN_A(NAME##_compare_exchange_weak, a, c, v);            \
        capture_simple((r ? CAT_AFTER_CMPXCHG_S : CAT_AFTER_CMPXCHG_F),        \
                       #NAME "_compare_exchange_weak", arg_ptr(a),             \
                       arg(size_t, SIZE));                                     \
        return r;                                                              \
    }                                                                          \
    static inline T __lotto_##NAME##_compare_exchange_val(volatile T *a, T c,  \
                                                          T v)                 \
    {                                                                          \
        (void)__lotto_##NAME##_compare_exchange_strong(a, &c, v);              \
        return c;                                                              \
    }                                                                          \
    TSAN_DECL(T, NAME##_compare_exchange_val, volatile T *a, T c, T v,         \
              __tsan_memory_order mo, __tsan_memory_order fmo)                 \
    {                                                                          \
        capture_simple(CAT_BEFORE_CMPXCHG, #NAME "_compare_exchange_val",      \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        T r = CALL_TSAN_A(NAME##_compare_exchange_val, a, c, v);               \
        capture_simple(CAT_AFTER_CMPXCHG_S, #NAME "_compare_exchange_val",     \
                       arg_ptr(a), arg(size_t, SIZE));                         \
        return r;                                                              \
    }

TSAN_DECL_CMPXCHG(uint8_t, atomic8, 1)
TSAN_DECL_CMPXCHG(uint16_t, atomic16, 2)
TSAN_DECL_CMPXCHG(uint32_t, atomic32, 4)
TSAN_DECL_CMPXCHG(uint64_t, atomic64, 8)
#undef TSAN_DECL_CMPXCHG

TSAN_DECL(void, atomic_thread_fence, __tsan_memory_order mo)
{
    capture_simple(CAT_BEFORE_FENCE, "atomic_thread_fence",
                   arg_ptr(NULL), arg(size_t, 0));
    capture_simple(CAT_AFTER_FENCE, "atomic_thread_fence",
                   arg_ptr(NULL), arg(size_t, 0));
    CALL_TSAN(atomic_thread_fence, mo);
}

TSAN_DECL(void, atomic_signal_fence, __tsan_memory_order mo)
{
    capture_simple(CAT_BEFORE_FENCE, "atomic_signal_fence",
                   arg_ptr(NULL), arg(size_t, 0));
    capture_simple(CAT_AFTER_FENCE, "atomic_signal_fence",
                   arg_ptr(NULL), arg(size_t, 0));
    CALL_TSAN(atomic_signal_fence, mo);
}

#undef TSAN_DECL
#undef CALL_TSAN
#undef CALL_TSAN_A
#undef CAT_BEFORE
#undef CAT_AFTER
#ifdef LOTTO_INTERCEPT_TSAN_DEFINED_V_JOIN
    #undef V_JOIN
    #undef LOTTO_INTERCEPT_TSAN_DEFINED_V_JOIN
#endif

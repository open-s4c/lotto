/*
 */

#ifndef LOTTO_SIG_DEFAULTS_WRAP_H
#define LOTTO_SIG_DEFAULTS_WRAP_H

#include <lotto/sys/real.h>
#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/util/prototypes.h>
#include <vsync/common/macros.h>

/*******************************************************************************
 * Internal functions to bypass libtsan
 ******************************************************************************/
#define SYS_FUNC_WRAP(LIB, R, S, ATTR)                                         \
    SYS_FUNC_HEAD(S, ATTR)                                                     \
    {                                                                          \
        REAL_##LIB##_INIT(RET_TYPE(S), FUNC_##S, ARGS_TYPEVARS(S));            \
        return REAL(FUNC(S), ARGS_VARS(S));                                    \
    }

#define RL_FUNC_WRAP(LIB, R, S, ATTR)                                          \
    SYS_FUNC_HEAD(S, ATTR __attribute__((weak)));                              \
    RL_FUNC_HEAD(S, ATTR static inline)                                        \
    {                                                                          \
        if (NULL != WRAP(S)) {                                                 \
            R WRAP(S)(ARGS_VARS(S));                                           \
        } else {                                                               \
            R FUNC(S)(ARGS_VARS(S));                                           \
        }                                                                      \
    }

/* *****************************************************************************
 * format functions (all from LIBC)
 * ****************************************************************************/
#define SYS_FORMAT_FUNC_WRAP(R, S, ATTR)                                       \
    SYS_FORMAT_FUNC_HEAD(S, ATTR)                                              \
    {                                                                          \
        REAL_APPLY(DECL, RET_TYPE(S), VFUNC(S), ARGS_TYPEVARS(S), ...);        \
        if (REAL_APPLY(NAME, VFUNC(S)) == NULL) {                              \
            REAL_APPLY(NAME, VFUNC(S)) =                                       \
                real_func(REAL_APPLY(STR, VFUNC(S)), REAL_LIBC);               \
        }                                                                      \
        ASSERT(REAL_APPLY(NAME, VFUNC(S)) != NULL);                            \
        va_list args;                                                          \
        va_start(args, fmt); /* assume there is argument fmt */                \
        RET_TYPE(S) r = REAL(VFUNC(S), ARGS_VARS(S), args);                    \
        va_end(args);                                                          \
        return r;                                                              \
    }

#define RL_FORMAT_FUNC_WRAP(R, S, ATTR)                                        \
    SYS_FORMAT_VFUNC_HEAD(S, __attribute__((weak)));                           \
    RL_FORMAT_FUNC_HEAD(S, ATTR static inline)                                 \
    {                                                                          \
        RET_TYPE(S) r;                                                         \
        va_list args;                                                          \
        va_start(args, fmt); /* assume there is argument fmt */                \
        if (NULL != VWRAP(S)) {                                                \
            r = VWRAP(S)(ARGS_VARS(S), args);                                  \
        } else {                                                               \
            r = VFUNC(S)(ARGS_VARS(S), args);                                  \
        }                                                                      \
        va_end(args);                                                          \
        return r;                                                              \
    }


#endif // LOTTO_SIG_DEFAULTS_WRAP_H

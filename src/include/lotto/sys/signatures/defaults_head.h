/*
 */

#ifndef LOTTO_SIG_DEFAULTS_HEAD_H
#define LOTTO_SIG_DEFAULTS_HEAD_H

#include <lotto/util/macros.h>
#include <lotto/util/prototypes.h>

/* fixed argument size function headers */

#define SYS_FUNC_HEAD(S, ATTR)   ATTR RET_TYPE(S) WRAP(S)(ARGS_TYPEVARS(S))
#define PLAIN_FUNC_HEAD(S, ATTR) ATTR RET_TYPE(S) FUNC(S)(ARGS_TYPEVARS(S))
#define RL_FUNC_HEAD(S, ATTR)    ATTR RET_TYPE(S) RL(S)(ARGS_TYPEVARS(S))

/* format printf-like function headers */

#define SYS_FORMAT_FUNC_HEAD(S, ATTR)                                          \
    ATTR RET_TYPE(S) WRAP(S)(ARGS_TYPEVARS(S), ...)

#define SYS_FORMAT_VFUNC_HEAD(S, ATTR)                                         \
    ATTR RET_TYPE(S) VWRAP(S)(ARGS_TYPEVARS(S), va_list arg)

#define PLAIN_FORMAT_FUNC_HEAD(S, ATTR)                                        \
    ATTR RET_TYPE(S) FUNC(S)(ARGS_TYPEVARS(S), ...)

#define RL_FORMAT_FUNC_HEAD(S, ATTR)                                           \
    ATTR RET_TYPE(S) RL(S)(ARGS_TYPEVARS(S), ...)

#define PLF_ENABLE(i) __attribute__((format(printf, i, (i + 1))))
#define PLF_DISABLE(i)

#define SLF_ENABLE(i) __attribute__((format(scanf, i, (i + 1))))
#define SLF_DISABLE(i)

#endif // LOTTO_SIG_DEFAULTS_HEAD_H

/*
 */

#ifndef LOTTO_SIGNATURES_STDLIB_H
#define LOTTO_SIGNATURES_STDLIB_H

#include <stdlib.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_ABORT SYS_FUNC(LIBC, , SIG(void, abort, void), NORETURN)
#define SYS_STRTOL                                                             \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(long, strtol, const char *, nptr, char **, endptr, int, base), )
#define SYS_STRTOLL                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(long long, strtoll, const char *, nptr, char **, endptr, int, \
                 base), )
#define SYS_MALLOC SYS_FUNC(LIBC, return, SIG(void *, malloc, size_t, n), )
#define SYS_CALLOC                                                             \
    SYS_FUNC(LIBC, return, SIG(void *, calloc, size_t, n, size_t, s), )
#define SYS_REALLOC                                                            \
    SYS_FUNC(LIBC, return, SIG(void *, realloc, void *, p, size_t, n), )
#define SYS_FREE SYS_FUNC(LIBC, , SIG(void, free, void *, p), )
#define SYS_EXIT SYS_FUNC(LIBC, , SIG(void, exit, int, status), )
#define SYS_SYSTEM                                                             \
    SYS_FUNC(LIBC, return, SIG(int, system, const char *, command), )

#define SYS_SETENV                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, setenv, const char *, name, const char *, value, int,    \
                 overwrite), )
#define SYS_UNSETENV                                                           \
    SYS_FUNC(LIBC, return, SIG(int, unsetenv, const char *, name), )

#define SYS_GETENV                                                             \
    SYS_FUNC(LIBC, return, SIG(char *, getenv, const char *, name), )

#define FOR_EACH_SYS_STDLIB_WRAPPED                                            \
    SYS_STRTOL                                                                 \
    SYS_STRTOLL                                                                \
    SYS_EXIT                                                                   \
    SYS_SYSTEM                                                                 \
    SYS_SETENV                                                                 \
    SYS_UNSETENV                                                               \
    SYS_GETENV

#define FOR_EACH_SYS_STDLIB_CUSTOM                                             \
    SYS_MALLOC                                                                 \
    SYS_CALLOC                                                                 \
    SYS_REALLOC                                                                \
    SYS_FREE                                                                   \
    SYS_ABORT

#define FOR_EACH_SYS_STDLIB                                                    \
    FOR_EACH_SYS_STDLIB_WRAPPED                                                \
    FOR_EACH_SYS_STDLIB_CUSTOM

#endif // LOTTO_SIGNATURES_STDLIB_H

/*
 */

#ifndef LOTTO_SIGNATURES_STRING_H
#define LOTTO_SIGNATURES_STRING_H

#include <string.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_MEMCPY                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(void *, memcpy, void *, dst, const void *, src, size_t, n), )
#define SYS_MEMSET                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(void *, memset, void *, dst, int, c, size_t, n), )
#define SYS_STRDUP                                                             \
    SYS_FUNC(LIBC, return, SIG(char *, strdup, const char *, str), )
#define SYS_STRNDUP                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(char *, strndup, const char *, str, size_t, len), )

#define SYS_STPCPY                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(char *, stpcpy, char *, dst, const char *, src), )
#define SYS_STRCPY                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(char *, strcpy, char *, dst, const char *, src), )
#define SYS_STRCAT                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(char *, strcat, char *, dst, const char *, src), )

#define SYS_STRCMP                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, strcmp, const char *, s1, const char *, s2), )
#define SYS_STRNCMP                                                            \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, strncmp, const char *, s1, const char *, s2, size_t, n), )
#define SYS_MEMCMP                                                             \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, memcmp, const void *, s1, const void *, s2, size_t, n), )
#define SYS_STRLEN                                                             \
    SYS_FUNC(LIBC, return, SIG(size_t, strlen, const char *, s), )

#define SYS_MEMMOVE                                                            \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(void *, memmove, void *, dest, const void *, src, size_t, n), )
#define FOR_EACH_SYS_STRING_WRAPPED                                            \
    SYS_MEMCPY                                                                 \
    SYS_MEMSET                                                                 \
    SYS_STPCPY                                                                 \
    SYS_STRCPY                                                                 \
    SYS_STRCAT                                                                 \
    SYS_STRCMP                                                                 \
    SYS_STRNCMP                                                                \
    SYS_MEMCMP                                                                 \
    SYS_STRLEN                                                                 \
    SYS_MEMMOVE

#define FOR_EACH_SYS_STRING_CUSTOM                                             \
    SYS_STRDUP                                                                 \
    SYS_STRNDUP

#define FOR_EACH_SYS_STRING                                                    \
    FOR_EACH_SYS_STRING_WRAPPED                                                \
    FOR_EACH_SYS_STRING_CUSTOM

#endif // LOTTO_SIGNATURES_STRING_H

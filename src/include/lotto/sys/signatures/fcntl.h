/*
 */

#ifndef LOTTO_SIGNATURES_FCNTL_H
#define LOTTO_SIGNATURES_FCNTL_H

#include <fcntl.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_OPEN                                                               \
    SYS_FUNC(                                                                  \
        LIBC, return,                                                          \
        SIG(int, open, const char *, pathname, int, flags, mode_t, mode), )

#define FOR_EACH_SYS_FCNTL_WRAPPED SYS_OPEN

#define FOR_EACH_SYS_FCNTL_CUSTOM

#define FOR_EACH_SYS_FCNTL                                                     \
    FOR_EACH_SYS_FCNTL_WRAPPED                                                 \
    FOR_EACH_SYS_FCNTL_CUSTOM

#endif // LOTTO_SIGNATURES_FCNTL_H

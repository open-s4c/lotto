/*
 */

#ifndef LOTTO_SIGNATURES_RANDOM_H
#define LOTTO_SIGNATURES_RANDOM_H

#include <lotto/sys/signatures/defaults_head.h>
#include <sys/random.h>

#define SYS_GETRANDOM                                                          \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(ssize_t, getrandom, void *, buf, size_t, buflen,              \
                 unsigned int, flags), )

#define FOR_EACH_SYS_RANDOM_WRAPPED SYS_GETRANDOM

#define FOR_EACH_SYS_RANDOM_CUSTOM

#define FOR_EACH_SYS_RANDOM                                                    \
    FOR_EACH_SYS_RANDOM_WRAPPED                                                \
    FOR_EACH_SYS_RANDOM_CUSTOM

#endif // LOTTO_SIGNATURES_RANDOM_H

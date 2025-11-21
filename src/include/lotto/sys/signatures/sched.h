/*
 */

#ifndef LOTTO_SIGNATURES_SCHED_H
#define LOTTO_SIGNATURES_SCHED_H

#ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
#endif
#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <sched.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_SCHED_YIELD SYS_FUNC(LIBC, return, SIG(int, sched_yield, void), )
#define SYS_SCHED_SETAFFINITY                                                  \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, sched_setaffinity, __pid_t, pid, size_t, s,              \
                 const cpu_set_t *, cset), )

#define FOR_EACH_SYS_SCHED_WRAPPED                                             \
    SYS_SCHED_YIELD                                                            \
    SYS_SCHED_SETAFFINITY

#define FOR_EACH_SYS_SCHED_CUSTOM

#define FOR_EACH_SYS_SCHED                                                     \
    FOR_EACH_SYS_SCHED_WRAPPED                                                 \
    FOR_EACH_SYS_SCHED_CUSTOM

#endif // LOTTO_SIGNATURES_SCHED_H

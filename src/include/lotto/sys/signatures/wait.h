/*
 */

#ifndef LOTTO_SIGNATURES_WAIT_H
#define LOTTO_SIGNATURES_WAIT_H

#include <lotto/sys/signatures/defaults_head.h>
#include <sys/wait.h>

#define SYS_WAITPID                                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(pid_t, waitpid, pid_t, pid, int *, stat_loc, int, options), )

#define FOR_EACH_SYS_WAIT_WRAPPED SYS_WAITPID

#define FOR_EACH_SYS_WAIT_CUSTOM

#define FOR_EACH_SYS_WAIT                                                      \
    FOR_EACH_SYS_WAIT_WRAPPED                                                  \
    FOR_EACH_SYS_WAIT_CUSTOM

#endif // LOTTO_SIGNATURES_WAIT_H

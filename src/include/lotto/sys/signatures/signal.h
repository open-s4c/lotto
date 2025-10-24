/*
 */

#ifndef LOTTO_SIGNATURES_SIGNAL_H
#define LOTTO_SIGNATURES_SIGNAL_H

#include <signal.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_KILL SYS_FUNC(LIBC, return, SIG(int, kill, pid_t, pid, int, sig), )
#define SYS_SIGADDSET                                                          \
    SYS_FUNC(LIBC, return, SIG(int, sigaddset, sigset_t *, set, int, signo), )
#define SYS_SIGEMPTYSET                                                        \
    SYS_FUNC(LIBC, return, SIG(int, sigemptyset, sigset_t *, set), )
#define SYS_SIGACTION                                                          \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, sigaction, int, sig, const struct sigaction *, act,      \
                 struct sigaction *, oact), )

#define FOR_EACH_SYS_SIGNAL_WRAPPED                                            \
    SYS_KILL                                                                   \
    SYS_SIGADDSET                                                              \
    SYS_SIGEMPTYSET                                                            \
    SYS_SIGACTION

#define FOR_EACH_SYS_SIGNAL_CUSTOM

#define FOR_EACH_SYS_SIGNAL                                                    \
    FOR_EACH_SYS_SIGNAL_WRAPPED                                                \
    FOR_EACH_SYS_SIGNAL_CUSTOM

#endif // LOTTO_SIGNATURES_SIGNAL_H

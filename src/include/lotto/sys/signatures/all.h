/*
 */

#ifndef LOTTO_SIG_ALL_H
#define LOTTO_SIG_ALL_H

#include <lotto/sys/signatures/fcntl.h>
#include <lotto/sys/signatures/poll.h>
#include <lotto/sys/signatures/pthread.h>
#include <lotto/sys/signatures/random.h>
#include <lotto/sys/signatures/sched.h>
#include <lotto/sys/signatures/signal.h>
#include <lotto/sys/signatures/socket.h>
#include <lotto/sys/signatures/spawn.h>
#include <lotto/sys/signatures/stdio.h>
#include <lotto/sys/signatures/stdlib.h>
#include <lotto/sys/signatures/string.h>
#include <lotto/sys/signatures/time.h>
#include <lotto/sys/signatures/unistd.h>
#include <lotto/sys/signatures/wait.h>

/* *****************************************************************************
 * Collecting all wrapped functions
 * ****************************************************************************/
#define FOR_EACH_SYS_FUNC_WRAPPED                                              \
    FOR_EACH_SYS_FCNTL_WRAPPED                                                 \
    FOR_EACH_SYS_POLL_WRAPPED                                                  \
    FOR_EACH_SYS_PTHREAD_WRAPPED                                               \
    FOR_EACH_SYS_RANDOM_WRAPPED                                                \
    FOR_EACH_SYS_SCHED_WRAPPED                                                 \
    FOR_EACH_SYS_SOCKET_WRAPPED                                                \
    FOR_EACH_SYS_STDIO_WRAPPED                                                 \
    FOR_EACH_SYS_STDLIB_WRAPPED                                                \
    FOR_EACH_SYS_STRING_WRAPPED                                                \
    FOR_EACH_SYS_TIME_WRAPPED                                                  \
    FOR_EACH_SYS_UNISTD_WRAPPED                                                \
    FOR_EACH_SYS_SIGNAL_WRAPPED                                                \
    FOR_EACH_SYS_SPAWN_WRAPPED                                                 \
    FOR_EACH_SYS_WAIT_WRAPPED


/* *****************************************************************************
 * Collecting all custom sys_ functions
 * ****************************************************************************/
#define FOR_EACH_SYS_FUNC_CUSTOM                                               \
    FOR_EACH_SYS_FCNTL_CUSTOM                                                  \
    FOR_EACH_SYS_POLL_CUSTOM                                                   \
    FOR_EACH_SYS_PTHREAD_CUSTOM                                                \
    FOR_EACH_SYS_RANDOM_CUSTOM                                                 \
    FOR_EACH_SYS_SCHED_CUSTOM                                                  \
    FOR_EACH_SYS_SOCKET_CUSTOM                                                 \
    FOR_EACH_SYS_STDIO_CUSTOM                                                  \
    FOR_EACH_SYS_STDLIB_CUSTOM                                                 \
    FOR_EACH_SYS_STRING_CUSTOM                                                 \
    FOR_EACH_SYS_TIME_CUSTOM                                                   \
    FOR_EACH_SYS_UNISTD_CUSTOM                                                 \
    FOR_EACH_SYS_SIGNAL_CUSTOM                                                 \
    FOR_EACH_SYS_SPAWN_CUSTOM                                                  \
    FOR_EACH_SYS_WAIT_CUSTOM

#define FOR_EACH_SYS_FUNC                                                      \
    FOR_EACH_SYS_FUNC_WRAPPED                                                  \
    FOR_EACH_SYS_FUNC_CUSTOM

#endif // LOTTO_SIG_ALL_H

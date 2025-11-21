/*
 */

#ifndef LOTTO_SIGNATURES_SPAWN_H
#define LOTTO_SIGNATURES_SPAWN_H

#include <signal.h>
#include <spawn.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_POSIX_SPAWNATTR_INIT                                               \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawnattr_init, posix_spawnattr_t *, attr), )
#define SYS_POSIX_SPAWNATTR_DESTROY                                            \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawnattr_destroy, posix_spawnattr_t *, attr), )

#define SYS_POSIX_SPAWNATTR_SETFLAGS                                           \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawnattr_setflags, posix_spawnattr_t *, attr,     \
                 short, flags), )
#define SYS_POSIX_SPAWNATTR_SETSIGDEFAULT                                      \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawnattr_setsigdefault, posix_spawnattr_t *,      \
                 attr, const sigset_t *, sigdefault), )

#define SYS_POSIX_SPAW_FILE_ACTIONS_DESTROY                                    \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawn_file_actions_destroy,                        \
                 posix_spawn_file_actions_t *, file_actions), )
#define SYS_POSIX_SPAW_FILE_ACTIONS_INIT                                       \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawn_file_actions_init,                           \
                 posix_spawn_file_actions_t *, file_actions), )

#define SYS_POSIX_SPAW_FILE_ACTIONS_ADDCLOSE                                   \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawn_file_actions_addclose,                       \
                 posix_spawn_file_actions_t *, file_actions, int, fildes), )
#define SYS_POSIX_SPAW_FILE_ACTIONS_ADDOPEN                                    \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawn_file_actions_addopen,                        \
                 posix_spawn_file_actions_t *, file_actions, int, fildes,      \
                 const char *, path, int, oflag, mode_t, mode), )

#define SYS_POSIX_SPAW_FILE_ACTIONS_ADDDUP2                                    \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, posix_spawn_file_actions_adddup2,                        \
                 posix_spawn_file_actions_t *, file_actions, int, fildes, int, \
                 newfildes), )

#define FOR_EACH_SYS_SPAWN_WRAPPED                                             \
    SYS_POSIX_SPAWNATTR_INIT                                                   \
    SYS_POSIX_SPAWNATTR_DESTROY                                                \
    SYS_POSIX_SPAWNATTR_SETFLAGS                                               \
    SYS_POSIX_SPAWNATTR_SETSIGDEFAULT                                          \
    SYS_POSIX_SPAW_FILE_ACTIONS_DESTROY                                        \
    SYS_POSIX_SPAW_FILE_ACTIONS_INIT                                           \
    SYS_POSIX_SPAW_FILE_ACTIONS_ADDCLOSE                                       \
    SYS_POSIX_SPAW_FILE_ACTIONS_ADDOPEN                                        \
    SYS_POSIX_SPAW_FILE_ACTIONS_ADDDUP2


#define FOR_EACH_SYS_SPAWN_CUSTOM

#define FOR_EACH_SYS_SPAWN                                                     \
    FOR_EACH_SYS_SPAWN_WRAPPED                                                 \
    FOR_EACH_SYS_SPAWN_CUSTOM

#endif // LOTTO_SIGNATURES_SPAWN_H

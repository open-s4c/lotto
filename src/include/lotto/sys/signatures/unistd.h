/*
 */

#ifndef LOTTO_SIGNATURES_UNISTD_H
#define LOTTO_SIGNATURES_UNISTD_H

#include <unistd.h>

#include <lotto/sys/signatures/defaults_head.h>

#define SYS_FORK SYS_FUNC(LIBC, return, SIG(pid_t, fork, void), )
#define SYS_EXECVE                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, execve, const char *, pathname, char *const *, argv,     \
                 char *const *, envp), )
#define SYS_DUP2                                                               \
    SYS_FUNC(LIBC, return, SIG(int, dup2, int, oldfd, int, newfd), )
#define SYS_SLEEP                                                              \
    SYS_FUNC(LIBC, return, SIG(unsigned int, sleep, unsigned int, seconds), )
#define SYS_USLEEP SYS_FUNC(LIBC, return, SIG(int, usleep, useconds_t, usec), )
#define SYS_CLOSE  SYS_FUNC(LIBC, return, SIG(int, close, int, fd), )
#define SYS_READ                                                               \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(ssize_t, read, int, fd, void *, buf, size_t, count), )
#define SYS_WRITE                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(ssize_t, write, int, fd, const void *, buf, size_t, count), )
#define SYS_LSEEK                                                              \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(off_t, lseek, int, fd, off_t, offset, int, whence), )

#define SYS_GETPID  SYS_FUNC(LIBC, return, SIG(pid_t, getpid, void), )
#define SYS_GETPPID SYS_FUNC(LIBC, return, SIG(pid_t, getppid, void), )

#define SYS_PIPE SYS_FUNC(LIBC, return, SIG(int, pipe, int *, pipefd), )
#define FOR_EACH_SYS_UNISTD_WRAPPED                                            \
    SYS_FORK                                                                   \
    SYS_EXECVE                                                                 \
    SYS_DUP2                                                                   \
    SYS_SLEEP                                                                  \
    SYS_USLEEP                                                                 \
    SYS_CLOSE                                                                  \
    SYS_READ                                                                   \
    SYS_WRITE                                                                  \
    SYS_LSEEK                                                                  \
    SYS_GETPID                                                                 \
    SYS_GETPPID                                                                \
    SYS_PIPE

#define FOR_EACH_SYS_UNISTD_CUSTOM

#define FOR_EACH_SYS_UNISTD                                                    \
    FOR_EACH_SYS_UNISTD_WRAPPED                                                \
    FOR_EACH_SYS_UNISTD_CUSTOM

#endif // LOTTO_SIGNATURES_UNISTD_H

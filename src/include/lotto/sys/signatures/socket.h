/*
 */

#ifndef LOTTO_SIGNATURES_SOCKET_H
#define LOTTO_SIGNATURES_SOCKET_H

#include <lotto/sys/signatures/defaults_head.h>
#include <sys/socket.h>

#define SYS_ACCEPT                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, accept, int, sockfd, struct sockaddr *, addr,            \
                 socklen_t *, addrlen), )
#define SYS_BIND                                                               \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, bind, int, sockfd, const struct sockaddr *, addr,        \
                 socklen_t, addrlen), )
#define SYS_LISTEN                                                             \
    SYS_FUNC(LIBC, return, SIG(int, listen, int, sockfd, int, backlog), )
#define SYS_SETSOCKOPT                                                         \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, setsockopt, int, sockfd, int, level, int, optname,       \
                 const void *, optval, socklen_t, optlen), )
#define SYS_SOCKET                                                             \
    SYS_FUNC(LIBC, return,                                                     \
             SIG(int, socket, int, domain, int, type, int, protocol), )

#define FOR_EACH_SYS_SOCKET_WRAPPED                                            \
    SYS_ACCEPT                                                                 \
    SYS_BIND                                                                   \
    SYS_LISTEN                                                                 \
    SYS_SETSOCKOPT                                                             \
    SYS_SOCKET

#define FOR_EACH_SYS_SOCKET_CUSTOM

#define FOR_EACH_SYS_SOCKET                                                    \
    FOR_EACH_SYS_SOCKET_WRAPPED                                                \
    FOR_EACH_SYS_SOCKET_CUSTOM

#endif // LOTTO_SIGNATURES_SOCKET_H

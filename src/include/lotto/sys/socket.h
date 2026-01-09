/*
 */
#ifndef LOTTO_SOCKET_H
#define LOTTO_SOCKET_H

#include <lotto/sys/signatures.h>
#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/sys/signatures/socket.h>
#include <sys/socket.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_SOCKET

#undef SYS_FUNC

#endif

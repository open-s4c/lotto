/*
 */
#ifndef LOTTO_UNISTD_H
#define LOTTO_UNISTD_H

#include <lotto/sys/signatures/unistd.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_UNISTD

#undef SYS_FUNC

#endif

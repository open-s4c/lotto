/*
 */
#ifndef LOTTO_WAIT_H
#define LOTTO_WAIT_H

#include <lotto/sys/signatures/wait.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_WAIT

#undef SYS_FUNC

#endif

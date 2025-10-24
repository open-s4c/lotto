/*
 */
#ifndef LOTTO_TIME_H
#define LOTTO_TIME_H

#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/sys/signatures/time.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_TIME

#undef SYS_FUNC

#endif

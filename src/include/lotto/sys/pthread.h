/*
 */
#ifndef LOTTO_PTHREAD_H
#define LOTTO_PTHREAD_H

#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/sys/signatures/pthread.h>

#define SYS_FUNC(LIB, R, S, SUF) SYS_FUNC_HEAD(S, SUF);

FOR_EACH_SYS_PTHREAD

#undef SYS_FUNC

#endif

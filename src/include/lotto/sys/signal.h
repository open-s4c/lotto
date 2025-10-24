/*
 */
#ifndef LOTTO_SIGNAL_H
#define LOTTO_SIGNAL_H

#include <lotto/sys/signatures/signal.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_SIGNAL

#undef SYS_FUNC

#endif

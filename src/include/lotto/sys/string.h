#ifndef LOTTO_SYS_STRING_H
#define LOTTO_SYS_STRING_H

#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/sys/signatures/string.h>

#define SYS_FUNC(LIB, R, S, ATTR) SYS_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_STRING

#undef SYS_FUNC

#endif

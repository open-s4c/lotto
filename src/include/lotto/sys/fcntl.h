/*
 */
#ifndef LOTTO_FCNTL_H
#define LOTTO_FCNTL_H

#include <lotto/sys/signatures/defaults_head.h>
#include <lotto/sys/signatures/fcntl.h>

#define PLF(i) PLF_ENABLE(i)
#define SLF(i) SLF_ENABLE(i)

#define SYS_FUNC(LIB, R, S, ATTR)   SYS_FUNC_HEAD(S, ATTR);
#define SYS_FORMAT_FUNC(R, S, ATTR) SYS_FORMAT_FUNC_HEAD(S, ATTR);

FOR_EACH_SYS_FCNTL

#undef SYS_FUNC
#undef SYS_FORMAT_FUNC

#undef PLF
#undef SLF

#endif

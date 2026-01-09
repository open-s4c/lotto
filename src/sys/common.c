/*
 */

#include <lotto/sys/signatures/all.h>
#include <lotto/sys/signatures/defaults_wrap.h>

#define PLF(i)                      PLF_ENABLE(i)
#define SLF(i)                      SLF_ENABLE(i)
#define SYS_FUNC(LIB, R, S, ATTR)   SYS_FUNC_WRAP(LIB, R, S, ATTR)
#define SYS_FORMAT_FUNC(R, S, ATTR) SYS_FORMAT_FUNC_WRAP(R, S, ATTR)

FOR_EACH_SYS_FUNC_WRAPPED

#undef SYS_FUNC
#undef SYS_FORMAT_FUNC
#undef PLF
#undef SLF

#ifndef LOTTO_ABORT_H
#define LOTTO_ABORT_H

#include <lotto/util/macros.h>

NORETURN void sys_abort(void);
NORETURN void sys_assert_fail(const char *a, const char *file,
                              unsigned int line, const char *func);

#endif

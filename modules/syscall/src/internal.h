#ifndef LOTTO_MODULES_SYSCALL_INTERNAL_H
#define LOTTO_MODULES_SYSCALL_INTERNAL_H

#include <stdbool.h>

bool lotto_runtime_interceptors_ready(void);
long lotto_syscall(long number, ...);

#endif

#include <stddef.h>

#include "internal.h"
#include <linux/futex.h>
#include <sys/syscall.h>

long
lotto_futex(int *uaddr, int futex_op, int val)
{
    return lotto_syscall(SYS_futex, uaddr, futex_op, val, NULL, NULL, 0);
}

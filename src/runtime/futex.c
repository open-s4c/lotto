#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/futex.h>
#include <lotto/util/macros.h>
#include <sys/syscall.h>
#include <vsync/atomic.h>

WEAK long
lotto_futex(int *uaddr, int futex_op, int val)
{
    return syscall(SYS_futex, uaddr, futex_op, val, NULL, NULL, 0);
}

void
vfutex_wait(vatomic32_t *m, vuint32_t v)
{
    long s = lotto_futex((int *)m, FUTEX_WAIT, (int)v);

    if (s == -1 && errno == EAGAIN) {
        return;
    }

    if (s == -1 && errno != EAGAIN) {
        perror("futex_wait failed");
        exit(EXIT_FAILURE);
    }
}

void
vfutex_wake(vatomic32_t *m, vuint32_t nthreads)
{
    long s = lotto_futex((int *)m, FUTEX_WAKE, (int)nthreads);

    if (s == -1) {
        perror("futex_wake failed");
        exit(EXIT_FAILURE);
    }
}

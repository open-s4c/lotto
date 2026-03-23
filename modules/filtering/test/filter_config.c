// clang-format off
// RUN: echo "32=1" > %s.conf
// RUN: echo "33=1" >> %s.conf
// RUN: echo "34=1" >> %s.conf
// RUN: %lotto %stress -r 200 -F --filtering-config %s.conf -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *
thread1(void *arg)
{
    (void)arg;
    x++;
    x++;
    return 0;
}

void *
thread2(void *arg)
{
    (void)arg;
    assert(x != 1);
    return 0;
}

int
main()
{
    pthread_t t1, t2;
    pthread_create(&t1, 0, thread1, 0);
    pthread_create(&t2, 0, thread2, 0);
    pthread_join(t2, 0);
    pthread_join(t1, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

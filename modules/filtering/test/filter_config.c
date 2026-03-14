// clang-format off
// RUN: echo "BEFORE_AREAD=1" > %s.conf
// RUN: echo "BEFORE_AWRITE=1" >> %s.conf
// RUN: echo "BEFORE_RMW=1" >> %s.conf
// RUN: %lotto %stress -r 200 -F --filtering-config %s.conf -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *
thread1()
{
    x++;
    x++;
    return 0;
}

void *
thread2()
{
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

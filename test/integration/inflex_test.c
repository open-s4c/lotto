// clang-format off
// RUN: (! %lotto %stress -s random -- %b)
// RUN: %lotto %inflex -r 50
// RUN: %lotto %show 2>&1 | filecheck %s
// CHECK: RECORD 2
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

void *
adder()
{
    x++;
    return NULL;
}

void *
checker()
{
    // inflection point: after adder incremented
    assert(x == 0);
    return NULL;
}

int
main()
{
    x = 0;
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, adder, 0);
    pthread_create(&thread2, 0, checker, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

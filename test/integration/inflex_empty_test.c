// clang-format off
// RUN: (! %lotto %stress -r 1 -- %b)
// RUN: %lotto %inflex -r 1 -s random
// RUN: %lotto %show 2>&1 | filecheck %s
// CHECK: clock:{{ *0}}
// CHECK-NOT: clock:{{ *[^ 0]}}
// clang-format on

#include <assert.h>
#include <pthread.h>

int x = 0;

void *
adder()
{
    x++;
    return NULL;
}

int
main()
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, adder, 0);
    pthread_create(&thread2, 0, adder, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    assert(0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

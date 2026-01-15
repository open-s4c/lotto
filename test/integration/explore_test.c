// clang-format off
// RUN: %lotto %stress --record-granularity CHPT -r 1 -- %b || true
// RUN: (! %lotto %explore 3>&2 2>&1 1>&3) | tail | %check %s
// CHECK: assert failed {{.*}}/explore_test.c:{{[0-9]+}}: x != 0b11
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

atomic_int x = 0;

void *
adder()
{
    x++;
    x++;
    assert(x != 0b11);
    return NULL;
}

void *
multiplier()
{
    x *= 2;
    return NULL;
}

int
main()
{
    pthread_t multiplier_thread, adder_thread;
    pthread_create(&adder_thread, 0, adder, 0);
    pthread_create(&multiplier_thread, 0, multiplier, 0);

    pthread_join(adder_thread, 0);
    pthread_join(multiplier_thread, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

// clang-format off
// RUN: (! %lotto %stress -r 1 -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/priority.c:{{[0-9]+}}: !(val == 0 && data == 1)
// clang-format on

#include <assert.h>
#include <pthread.h>

#include <lotto/priority.h>

volatile int data;
volatile int ctrl;

void *
run_alice(void *arg)
{
    ctrl = 1;
    lotto_priority(-2);
    data = 1;
    return NULL;
}

void *
run_bob(void *arg)
{
    lotto_priority(-1);
    if (!ctrl)
        return NULL;
    int val = data;
    lotto_priority(-3);

    assert(!(val == 0 && data == 1));
    return NULL;
}

int
main()
{
    pthread_t alice, bob;
    lotto_priority(1);
    pthread_create(&alice, 0, run_alice, 0);
    pthread_create(&bob, 0, run_bob, 0);
    pthread_join(alice, 0);
    pthread_join(bob, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

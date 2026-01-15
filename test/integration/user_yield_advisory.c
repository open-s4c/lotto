// clang-format off
// RUN: (! %lotto %stress --advisory-yield -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/user_yield_advisory.c:{{[0-9]+}}: val == 0
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

int data;
int ctrl;

void *
run_alice(void *arg)
{
    (void)arg;
    ctrl = 1;
    sched_yield();
    data = 1;
    return NULL;
}

void *
run_bob(void *arg)
{
    (void)arg;
    while (!ctrl)
        ;
    int val = data;

    assert(val == 0);

    return NULL;
}

int
main()
{
    pthread_t alice, bob;
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

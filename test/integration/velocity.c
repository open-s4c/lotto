// clang-format off
// RUN: (! %lotto %stress -s random -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/velocity.c:{{[0-9]+}}: atomic_load(&x) == 1
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/velocity.h>

volatile atomic_int x = 0;

void *
run_alice(void *arg)
{
    lotto_task_velocity(10);
    atomic_store(&x, 1);
    return NULL;
}

void *
run_bob(void *arg)
{
    int i = 0;
    do {
        i++;
        atomic_fetch_add(&x, 0);
    } while (i < 10);
    assert(atomic_load(&x) == 1);
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

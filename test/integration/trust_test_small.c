// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable -r 100 -- %b 2>&1) | filecheck %s
// CHECK: Total number of execution graphs: 4
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

_Atomic unsigned a = 0;
_Atomic unsigned b = 0;

void *
run_alice(void *arg)
{
    a = 1;
    if (b == 1) {
        a = 2;
    } else {
        a = 3;
    }
    return NULL;
}

void *
run_bob(void *arg)
{
    if (a == 1) {
        b = 1;
    }

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

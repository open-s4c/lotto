// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable -- %b 2>&1) | %check %s
// CHECK: Total number of execution graphs: 89
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
    atomic_fetch_add(&a, 1);
    atomic_fetch_add(&a, 2);
    if (a > 3) {
        atomic_store(&a, 0);
    }
    return NULL;
}

void *
run_bob(void *arg)
{
    atomic_fetch_xor(&a, 3);
    if (a == 3) {
        atomic_store(&a, 4);
    }
    if (b) {
        atomic_load(&a);
        atomic_fetch_or(&a, 3);
        atomic_load(&b);
    }
    return NULL;
}

void *
run_charlie(void *arg)
{
    if (a == 7) {
        b = 1;
        atomic_store(&a, 0);
    }
    return NULL;
}

int
main()
{
    pthread_t alice, bob, charlie;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_create(&bob, 0, run_bob, 0);
    pthread_create(&charlie, 0, run_charlie, 0);

    pthread_join(alice, 0);
    pthread_join(bob, 0);
    pthread_join(charlie, 0);
    return 0;
}

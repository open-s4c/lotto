// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable -r 10000 -- %b 2>&1) | %check %s
// CHECK: Total number of execution graphs: 640
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
    unsigned exp = 0;
    atomic_compare_exchange_strong(&a, &exp, 2);
    exp = 0;
    atomic_compare_exchange_strong(&a, &exp, 2);
    atomic_load(&b);
    atomic_store(&a, 2);
    return NULL;
}

void *
run_bob(void *arg)
{
    atomic_store(&a, 1);
    unsigned exp = 1;
    atomic_load(&a);
    atomic_compare_exchange_strong(&a, &exp, 2);
    atomic_load(&a);
    atomic_store(&b, 2);
    return NULL;
}

void *
run_charlie(void *arg)
{
    atomic_store(&b, 1);
    atomic_load(&a);
    unsigned exp = 2;
    atomic_compare_exchange_strong(&b, &exp, 0);
    atomic_store(&b, 1);
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

// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable -r 100 -- %b 2>&1) | filecheck %s
// CHECK: Total number of execution graphs: 38
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

_Atomic unsigned a           = 1;
_Atomic unsigned b           = 1234321;
_Atomic unsigned long long c = 999;


void *
run_alice(void *arg)
{
    unsigned exp = 1;
    if (atomic_compare_exchange_strong(&a, &exp, 2)) {
        b = 0;
    }
    exp = 0;
    if (atomic_compare_exchange_strong(&b, &exp, 0)) {
        a = 3;
    }
    unsigned long long exp_ll = 999;
    if (atomic_compare_exchange_strong(&c, &exp_ll, 1)) {
        a = 1234;
    }
    return NULL;
}

void *
run_bob(void *arg)
{
    unsigned exp = 1234321;
    if (atomic_compare_exchange_strong(&b, &exp, 1)) {
        a = 1234;
    }
    exp = 1234;
    if (atomic_compare_exchange_strong(&a, &exp, 2)) {
        b = 333;
    }
    unsigned long long exp_ll = 999;
    if (atomic_compare_exchange_strong(&c, &exp_ll, 1)) {
        b = 0;
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

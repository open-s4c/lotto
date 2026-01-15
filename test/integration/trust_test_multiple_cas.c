// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable -r 10000 -- %b 2>&1) | %check %s
// CHECK: Total number of execution graphs: 30
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
    atomic_compare_exchange_strong(&b, &exp, 2);
    return NULL;
}

int
main()
{
    const int N = 30;
    pthread_t t[N];
    for (int i = 0; i < N; i++) {
        pthread_create(&t[i], 0, run_alice, 0);
    }
    for (int i = 0; i < N; i++) {
        pthread_join(t[i], 0);
    }

    return 0;
}

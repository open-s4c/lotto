// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (! %lotto %stress --handler-trust enable --handler-race-strict -r 20 -- %b 2>&1)| filecheck %s --check-prefix=HANDLER
// HANDLER: {{\[.*handler_race.c\]}} Data race detected at addr: {{.*}}
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

_Atomic unsigned a = 0;
_Atomic unsigned b = 0;
int id             = 0;
int
get_id()
{
    return ++id;
}

void *
run_alice(void *arg)
{
    a = 1;
    if (b == 1) {
        a = 2;
        if (b == 2) {
            a = 3;
            get_id();
        }
    }
    return NULL;
}

void *
run_bob(void *arg)
{
    if (a == 1) {
        b = 1;
        if (a == 2) {
            b = 2;
            if (a == 3) {
                get_id();
            }
        }
    }
    return NULL;
}

int
main()
{
    // Without genmc this takes ~150 runs.
    // With genmc it can explore all the 13 graphs
    // very fast, and finds the bug in the third run.
    pthread_t alice, bob;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_create(&bob, 0, run_bob, 0);

    pthread_join(alice, 0);
    pthread_join(bob, 0);
    return 0;
}

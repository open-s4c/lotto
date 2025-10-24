// clang-format off
// ALLOW_RETRIES: 100
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: (%lotto stress -s random -r 1 -- %b 2>&1)
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#include <lotto/await.h>

_Atomic unsigned a            = 0;
_Atomic unsigned b            = 0;
_Atomic unsigned c            = 0;
_Atomic unsigned d            = 0;
_Atomic unsigned spin_counter = 0;

const unsigned MAX_ITERS   = 10U;
const unsigned NUM_THREADS = 4U;

void *
run_alice(void *arg)
{
    assert(a == 0);
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        while (d != i) {
            lotto_await(&d);
            // printf("alice is spinning in iter %u...\n", i);
            atomic_fetch_add_explicit(&spin_counter, 1, memory_order_relaxed);
        }
        a += 1;
    }

    return NULL;
}

void *
run_bob(void *arg)
{
    assert(b == 0);
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        while (a != i + 1) {
            lotto_await(&a);
            atomic_fetch_add_explicit(&spin_counter, 1, memory_order_relaxed);
            // printf("bob is spinning in iter %u...\n", i);
        }
        b += 1;
    }
    return NULL;
}

void *
run_charlie(void *arg)
{
    assert(c == 0);
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        while (b != i + 1) {
            lotto_await(&b);
            atomic_fetch_add_explicit(&spin_counter, 1, memory_order_relaxed);
            // printf("charlie is spinning in iter %u...\n", i);
        }
        c += 1;
    }
    return NULL;
}

void *
run_delta(void *arg)
{
    assert(d == 0);
    for (unsigned i = 0; i < MAX_ITERS; i++) {
        while (c != i + 1) {
            lotto_await(&c);
            atomic_fetch_add_explicit(&spin_counter, 1, memory_order_relaxed);
            // printf("delta is spinning in iter %u...\n", i);
        }
        d += 1;
    }
    return NULL;
}


int
main()
{
    assert(MAX_ITERS > 1);
    pthread_t alice, bob, charlie, delta;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_create(&bob, 0, run_bob, 0);
    pthread_create(&charlie, 0, run_charlie, 0);
    pthread_create(&delta, 0, run_delta, 0);

    pthread_join(alice, 0);
    pthread_join(bob, 0);
    pthread_join(charlie, 0);
    pthread_join(delta, 0);

    assert(a == MAX_ITERS);
    assert(b == MAX_ITERS);
    printf("Number of spins: %u\n", spin_counter);
    // The await annotation should prevent spinning
    assert(spin_counter <= (MAX_ITERS * NUM_THREADS));
    return 0;
}

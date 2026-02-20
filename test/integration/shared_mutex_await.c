// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: %lotto stress --handler-await-address enable --handler-spin-loop enable -s pos  -r 5 -- %b
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#include <lotto/await_while.h>

const unsigned MAX_ITERS          = 50U;
const unsigned NUM_THREADS        = 8U;
_Atomic unsigned val[4]           = {0, 0, 0, 0};
_Atomic unsigned finished_threads = 0, num_unlocks = 0, total_CAS = 0;
_Atomic unsigned total_XCHG = 0;

unsigned l = 0;
void
compare_swap_lock()
{
    unsigned wanted      = 0;
    register int num_CAS = 0;
    await_while (!__atomic_compare_exchange_n(
        &l, &wanted, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
        // assert(wanted == 1); // remove or else this read triggers
        // the loop to break because of a read-write to the same variable.
        // Could do it with lotto-ignore if needed
        wanted = 0;
        num_CAS++;
    }
    total_CAS += num_CAS;
}
void
exchange_lock()
{
    register int num_exchange = 0;
    await_while (__atomic_exchange_n(&l, 1, __ATOMIC_SEQ_CST) == 1) {
        num_exchange++;
    }
    total_XCHG += num_exchange;
}
void
unlock()
{
    num_unlocks += 1;
    assert(l == 1);
    l = 0;
}

void *
run_loops(void *arg)
{
    // int id = *(int *)arg;
    // printf("Starting thread %d\n", id);
    for (unsigned i = 0; i < MAX_ITERS; ++i) {
        compare_swap_lock();
        unlock();
        exchange_lock();
        unlock();
        // if (i % 5 == 0)
        //     printf("Thread %d is now in iter %u\n", id, i);
    }
    finished_threads += 1;
    return NULL;
}


int
main()
{
    pthread_t thr[NUM_THREADS];
    int ids[NUM_THREADS];
    val[0] = 1;
    for (unsigned i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&thr[i], 0, run_loops, (void *)&ids[i]);
    }
    for (unsigned i = 0; i < NUM_THREADS; i++)
        pthread_join(thr[i], 0);
    assert(finished_threads == NUM_THREADS);
    assert(num_unlocks == 2 * NUM_THREADS * MAX_ITERS);
    // printf("Total number of CAS: %d\n", total_CAS);
    // Worts case:
    // For each of the `MAX_ITERS * NUM_THREADS` total rounds
    // every thread spins and only one succeds.
    assert(total_CAS <= MAX_ITERS * NUM_THREADS * NUM_THREADS);
    // same for XCHG
    assert(total_XCHG <= MAX_ITERS * NUM_THREADS * NUM_THREADS);
    return 0;
}

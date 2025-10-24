// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: %lotto stress -s random -r 5 -- %b
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#include <lotto/await_while.h>

const unsigned MAX_ITERS          = 50U;
const unsigned NUM_THREADS        = 4U;
_Atomic unsigned val[4]           = {0, 0, 0, 0};
_Atomic unsigned finished_threads = 0;

void *
run_loops(void *arg)
{
    int id  = *(int *)arg;
    int nxt = (id + 1) % NUM_THREADS;
    // printf("starting %d, nxt = %d\n", id, nxt);
    for (unsigned i = 0; i < MAX_ITERS; ++i) {
        unsigned cnt = 0;
        await_while (val[nxt] != i + 1) {
            // printf("thread %d is spinning in iter %u..., nxt = %d\n", id, i,
            //        val[nxt]);
            cnt += 1;
        }
        if (cnt > 1) {
            // printf("Failed for Id = %d, i = %u, nxt = %d, cond = %d\n", id,
            // i,
            //        val[nxt], (val[nxt] != i + 1));
            assert(0);
        }
        // printf("Write from %d\n", id);
        val[id] += 1;
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
    return 0;
}

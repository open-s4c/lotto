// clang-format off
// RUN: %lotto %stress -r 100 -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

#include <lotto/unsafe/ghost.h>

#define NTHREADS 8

static int x;

static void *
thread(void *arg)
{
    (void)arg;

    lotto_ghost({
        int val = x;
        sched_yield();
        val = val + 1;
        x   = val;
    });

    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];

    for (int i = 0; i < NTHREADS; ++i) {
        pthread_create(&threads[i], NULL, thread, NULL);
    }

    for (int i = 0; i < NTHREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    assert(x == NTHREADS);
    return 0;
}

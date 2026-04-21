// clang-format off
// ALLOW_RETRIES: 5
// RUN: printf 'EVENT_MA_READ=0.99\nEVENT_MA_WRITE=0.99\n' > %t.conf
// RUN: %lotto %stress -r 1 --sampling-config %t.conf --enforce-mode SEED --record-granularity CAPTURE -- %b | grep '^shared=' > %t.record
// RUN: %lotto %replay | grep '^shared=' > %t.replay
// RUN: test -s %t.replay
// RUN: diff -u %t.record %t.replay
// clang-format on

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

enum {
    NTHREADS = 4,
    NITERS   = 1000,
};

static volatile int shared[NTHREADS];

static void *
worker(void *arg)
{
    intptr_t id = (intptr_t)arg;
    intptr_t a  = id;
    intptr_t b  = (id + 1) % NTHREADS;
    intptr_t c  = (id + 2) % NTHREADS;

    for (int i = 0; i < NITERS; i++) {
        int x     = shared[b];
        int y     = shared[c];
        shared[a] = x + y + i + (int)id;
        shared[b] = shared[a] ^ y;
    }
    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];

    for (int i = 0; i < NTHREADS; i++) {
        shared[i] = i + 1;
    }

    for (intptr_t i = 0; i < NTHREADS; i++) {
        pthread_create(&threads[i], NULL, worker, (void *)i);
    }
    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("shared=%d,%d,%d,%d\n", shared[0], shared[1], shared[2], shared[3]);
    return 0;
}

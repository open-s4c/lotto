// RUN: %lotto %record -- %x | grep LAST_THREAD | tee %x.record
// RUN: %lotto %replay -- %x | grep LAST_THREAD | tee %x.replay
// RUN: diff %x.record %x.replay
//
// We need TSAN instrumentation to run this with the watchdog OR we need
// libvsync annotation to information Lotto about the spinloop.

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include <vsync/atomic.h>

#define NTHREADS 3

static pthread_mutex_t mutex  = PTHREAD_MUTEX_INITIALIZER;
static volatile uint64_t last = 0;
static vatomic32_t start      = VATOMIC_INIT(0);
static vatomic32_t ready      = VATOMIC_INIT(0);

static void *
thread(void *arg)
{
    uint64_t v = (uint64_t)(uintptr_t)arg;

    vatomic_inc(&ready);
    vatomic_await_eq(&start, 1);

    pthread_mutex_lock(&mutex);
    last++; // just to change the value
    last = v;
    last++;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];

    for (int i = 0; i < NTHREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread, (void *)(uint64_t)i))
            return 1;
    }
    vatomic_await_eq(&ready, NTHREADS);
    vatomic_write(&start, 1);

    for (int i = 0; i < NTHREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    assert(last > 0 && last <= 100);

    printf("LAST_THREAD: %" PRIu64 "\n", last);

    return 0;
}

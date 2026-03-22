// REQUIRES: module-blocking
// RUN: %lotto %record -- %b | grep THREAD > %b.record 2> /dev/null
// RUN: %lotto %replay | grep THREAD > %b.replay 2> /dev/null
// RUN: diff %b.record %b.replay

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>

#include <dice/log.h>
#include <vsync/atomic.h>

#define NTHREADS 4

static void *
thread(void *arg)
{
    log_printf("THREAD %p\n", arg);
    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];

    for (int i = 0; i < NTHREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread,
                           (void *)(uint64_t)(i + 1)))
            return 1;
    }

    for (int i = 0; i < NTHREADS; ++i) {
        int err = pthread_join(threads[i], NULL);
        assert(err == 0);
    }

    return 0;
}

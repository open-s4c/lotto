// REQUIRES: module-blocking
// clang-format off
// RUN: %lotto --load-runtime %B/pthread_create-probe.so %record -- %b | grep -E 'THREAD|HANDLER:' > %b.record 2> /dev/null
// RUN: %lotto --load-runtime %B/pthread_create-probe.so %replay | grep -E 'THREAD|HANDLER:' > %b.replay 2> /dev/null
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>

#include <dice/log.h>
#include "probes/mock-probe.h"

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

    assert(lotto_test_handler_count() == NTHREADS);
    return 0;
}

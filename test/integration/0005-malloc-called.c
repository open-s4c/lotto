// REQUIRES: module-blocking
// RUN: %lotto --load-runtime %B/malloc-probe.so %record -- %b \
// RUN:        | grep THREAD > %b.record 2> /dev/null
// RUN: %lotto --load-runtime %B/malloc-probe.so %replay \
// RUN:        | grep THREAD > %b.replay 2> /dev/null
// RUN: diff %b.record %b.replay

#include <assert.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include <dice/log.h>

#define NTHREADS 4

enum {
    MALLOC_PROBE_SIZE = 65521,
};

typedef int (*probe_count_f)(void);

static void *
thread(void *arg)
{
    void *ptr = malloc(MALLOC_PROBE_SIZE);

    assert(ptr != NULL);
    log_printf("THREAD %p\n", arg);
    free(ptr);
    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];
    probe_count_f before_count =
        (probe_count_f)dlsym(RTLD_DEFAULT, "lotto_test_malloc_before_count");
    probe_count_f after_count =
        (probe_count_f)dlsym(RTLD_DEFAULT, "lotto_test_malloc_after_count");

    assert(before_count != NULL);
    assert(after_count != NULL);

    for (int i = 0; i < NTHREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread,
                           (void *)(uint64_t)(i + 1))) {
            return 1;
        }
    }

    for (int i = 0; i < NTHREADS; ++i) {
        int err = pthread_join(threads[i], NULL);
        assert(err == 0);
    }

    log_printf("COUNTS before=%d after=%d\n", before_count(), after_count());
    assert(before_count() == after_count());
    return 0;
}

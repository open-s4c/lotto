// REQUIRES: module-blocking
// clang-format off
// RUN: (%lotto --load-runtime %B/malloc-random-sigabort-probe.so %record -- %b; echo EXIT:$?) 2>&1 | grep -E 'THREAD|MALLOC_RANDOM:|EXIT:' > %b.record
// RUN: (%lotto --load-runtime %B/malloc-random-sigabort-probe.so %replay; echo EXIT:$?) 2>&1 | grep -E 'THREAD|MALLOC_RANDOM:|EXIT:' > %b.replay
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include <dice/log.h>

enum {
    MALLOC_RANDOM_SIGABORT_SIZE = 65521,
};

static void *
run(void *arg)
{
    void *ptr;

    (void)arg;
    ptr = malloc(MALLOC_RANDOM_SIGABORT_SIZE);
    assert(ptr != NULL);
    log_printf("THREAD malloc\n");
    free(ptr);
    return 0;
}

int
main(void)
{
    pthread_t thread;
    int err = pthread_create(&thread, 0, run, 0);

    assert(err == 0);
    err = pthread_join(thread, 0);
    assert(err == 0);
    return 0;
}

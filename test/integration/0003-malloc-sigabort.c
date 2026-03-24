// REQUIRES: module-trace, module-blocking
// RUN: (! %lotto --load-runtime %B/malloc-sigabort-probe.so %record -- %b 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: (! %lotto --load-runtime %B/malloc-sigabort-probe.so %replay 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: %lotto %show | %check %s --check-prefix=TRACE
// OUTPUT: SIGABRT
// TRACE: reason:   SIGABRT

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

enum {
    MALLOC_SIGABORT_SIZE = 65521,
};

static void *
run(void *arg)
{
    void *ptr;

    (void)arg;
    ptr = malloc(MALLOC_SIGABORT_SIZE);
    assert(ptr != NULL);
    free(ptr);
    return 0;
}

int
main(void)
{
    pthread_t thread;

    pthread_create(&thread, 0, run, 0);
    pthread_join(thread, 0);
    return 0;
}

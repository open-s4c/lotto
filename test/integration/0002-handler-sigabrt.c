// REQUIRES: module-trace, module-blocking
// RUN: (! %lotto --load-runtime %B/pthread_create-sigabrt-probe.so \
// RUN:           %record -- %b 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: (! %lotto --load-runtime %B/pthread_create-sigabrt-probe.so \
// RUN:           %replay 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: %lotto %show | %check %s --check-prefix=TRACE
// OUTPUT: RUNTIME_SIGABRT
// TRACE: reason:   RUNTIME_SIGABRT

#include <pthread.h>

static void *
run(void *arg)
{
    (void)arg;
    return 0;
}

int
main(void)
{
    pthread_t t;
    pthread_create(&t, 0, run, 0);
    pthread_join(t, 0);
    return 0;
}


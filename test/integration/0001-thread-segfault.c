// REQUIRES: module-trace
// RUN: (! %lotto %record -- %b 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: (! %lotto %replay 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: %lotto %show | %check %s --check-prefix=TRACE
// OUTPUT: SEGFAULT
// TRACE: reason:   SEGFAULT

#include <pthread.h>

#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("thread")))
#endif
static void *
run(void *arg)
{
    (void)arg;
    *(volatile int *)0 = 1;
    return 0;
}

int
main()
{
    pthread_t t;
    pthread_create(&t, 0, run, 0);
    pthread_join(t, 0);
    return 0;
}


// REQUIRES: module-trace
// RUN: (! %lotto %record -- %b 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: (! %lotto %replay 2>&1) | %check %s --check-prefix=OUTPUT
// RUN: %lotto %show | %check %s --check-prefix=TRACE
// OUTPUT: SIGABRT
// TRACE: reason:   SIGABRT

#include <pthread.h>
#include <signal.h>

static void *
run(void *arg)
{
    (void)arg;
    raise(SIGABRT);
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

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

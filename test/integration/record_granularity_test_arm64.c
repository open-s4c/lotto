// clang-format off
// UNSUPPORTED: x86_64
// RUN: %lotto %stress --handler-race disable -r 1 -- %b
// RUN: %lotto %show 2>&1 | filecheck %s --check-prefix=MINIMAL
// RUN: %lotto %stress --handler-race disable -r 1 --record-granularity CHPT -- %b
// RUN: %lotto %show 2>&1 | filecheck %s --check-prefix=CHPT
// RUN: %lotto %stress --handler-race disable -r 1 --record-granularity SWITCH -- %b
// RUN: %lotto %show 2>&1 | filecheck %s --check-prefix=SWITCH
// RUN: %lotto %stress --handler-race disable -r 1 --record-granularity CAPTURE -- %b
// RUN: %lotto %show 2>&1 | filecheck %s --check-prefix=CAPTURE
//
// MINIMAL: RECORD 2
// MINIMAL-NOT: RECORD 5
// CHPT: RECORD 6
// CHPT-NOT: RECORD 8
// SWITCH: RECORD 4
// SWITCH-NOT: RECORD 7
// CAPTURE: RECORD 14
// CAPTURE-NOT: RECORD 15

#include <pthread.h>

int x = 0;

void *
thread()
{
    x++;
    return NULL;
}

int
main()
{
    pthread_t thread1;
    pthread_create(&thread1, 0, thread, 0);
    pthread_join(thread1, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

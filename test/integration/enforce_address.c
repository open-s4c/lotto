// clang-format off
// ALLOW_RETRIES: 100
// RUN: %lotto %stress -r 1 --enforcement-mode ADDRESS --record-granularity CAPTURE -- %b
// RUN: sleep 1
// RUN: (! %lotto %replay 3>&2 2>&1 1>&3) | filecheck %s
// CHECK: MISMATCH [field: args[0].value.u64
// clang-format on

#include <pthread.h>
#include <time.h>

#include <lotto/unsafe/time.h>

volatile int x, y;

void *
run_alice(void *arg)
{
    time_t z = real_time(NULL) % 2 ? x : y;
    return z ? NULL : NULL;
}

int
main()
{
    pthread_t alice;
    x = y = 0;
    pthread_create(&alice, 0, run_alice, 0);
    pthread_join(alice, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

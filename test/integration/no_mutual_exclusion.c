// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/no_mutual_exclusion.c:{{[0-9]+}}: x == 1
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *
run()
{
    x = 1;
    assert(x == 1);
    return 0;
}

int
main()
{
    pthread_t t;
    x = 1234567;
    pthread_create(&t, 0, run, 0);
    x = 2;
    pthread_join(t, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

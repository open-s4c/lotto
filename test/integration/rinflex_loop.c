// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress -s random --stable-address-method MAP --handler-address enable --handler-cas enable --handler-stacktrace enable --handler-event enable -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -f utf-8 -t utf-8 -c | filecheck %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, 3 x pc: {{.*}}
// CHECK: event - tid: 3, clk: {{[0-9]+}}, 1 x pc: {{.*}}

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#define N_LOOPS 3

atomic_int x = 1;

void *
t1()
{
    for (int i = 0; i < N_LOOPS; ++i) {
        x *= 2;
    }
    return NULL;
}

void *
t2()
{
    x += 1;
    assert(x != (1 << N_LOOPS) + 1);
    return NULL;
}

int
main()
{
    pthread_t t1_t, t2_t;

    pthread_create(&t1_t, 0, t1, 0);
    pthread_create(&t2_t, 0, t2, 0);

    pthread_join(t1_t, 0);
    pthread_join(t2_t, 0);

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

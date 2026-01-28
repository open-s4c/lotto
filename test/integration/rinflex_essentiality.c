// clang-format off
// ALLOW_RETRIES: 5
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | filecheck %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[0-9]+}}, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: BEFORE_AWRITE
// CHECK-NEXT: [setter]
// CHECK: event - tid: {{[0-9]+}}, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: BEFORE_AREAD
// CHECK-NEXT: [checker]

// clang-format on

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

atomic_int flag = 0;

/* The number of virtual OCs is expected to be (N_SETTER - 1). */
#define N_CHECKER 1
#define N_SETTER  2
#define N         (N_CHECKER + N_SETTER)

void *
setter(void *arg)
{
    flag = 1;
    return NULL;
}

void *
checker(void *arg)
{
    assert(flag == 0);
    return NULL;
}

int
main()
{
    pthread_t t[N];
    for (int i = 0; i < N; ++i) {
        pthread_create(&t[i], NULL, i == 0 ? checker : setter, NULL);
    }
    for (int i = 0; i < N; ++i) {
        pthread_join(t[i], NULL);
    }
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

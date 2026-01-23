// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | %check %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AREAD

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: AFTER_CMPXCHG_S
// CHECK: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: AFTER_CMPXCHG_S

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x = 0;

void *
t1()
{
    x++;
    x++;
    return NULL;
}

void *
t2()
{
    x *= 2;
    assert(x != 0b11);
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

// b1:               s_id t_id

// x ++ -> x *= 2       2 3
// x *= 2 -> x ++       3 2

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

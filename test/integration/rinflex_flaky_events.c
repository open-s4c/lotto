// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -f utf-8 -t utf-8 -c | %check %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 4, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// CHECK: event - tid: 5, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// NOTE: Before control dependency is supported, the following
// IMPLICIT orderings are implied by the existence of the event above
// in tid 4.

// IMPLICIT: ------ . ------ . ------ . ------ . ------
// IMPLICIT-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// IMPLICIT: event - tid: 4, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// IMPLICIT: ------ . ------ . ------ . ------ . ------
// IMPLICIT-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// IMPLICIT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

atomic_int x = 0;
atomic_int z = 0;

void *
t1()
{
    x++;
    return NULL;
}
void *
t2()
{
    x *= 2;
    return NULL;
}
void *
t3()
{
    if (x == 2) {
        z++;
    }
    return NULL;
}
void *
t4()
{
    z *= 2;
    return NULL;
}

int
main()
{
    pthread_t t1_t, t2_t, t3_t, t4_t;

    pthread_create(&t1_t, 0, t1, 0);
    pthread_create(&t2_t, 0, t2, 0);
    pthread_create(&t3_t, 0, t3, 0);
    pthread_create(&t4_t, 0, t4, 0);

    pthread_join(t1_t, 0);
    pthread_join(t2_t, 0);
    pthread_join(t3_t, 0);
    pthread_join(t4_t, 0);

    assert(z != 2);

    return 0;
}

// b1:               s_id t_id

// x ++ -> x *= 2       4 5
// x *= 2 -> if (x)     5 7

// y ++ -> y *= 2       2 3
// y *= 2 -> if (y)     3 6

// z ++ -> z *= 2       6 7

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -f utf-8 -t utf-8 -c > %s.out

// RUN: ( %check %s --check-prefix=FIRST < %s.out ) || ( %check %s --check-prefix=SECOND < %s.out )

// FIRST: ------ . ------ . ------ . ------ . ------
// FIRST: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// FIRST: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// FIRST: ------ . ------ . ------ . ------ . ------
// FIRST: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// FIRST: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// SECOND: ------ . ------ . ------ . ------ . ------
// SECOND: event - tid: 5, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// SECOND: event - tid: 4, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// SECOND: ------ . ------ . ------ . ------ . ------
// SECOND: event - tid: 4, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}
// SECOND: event - tid: 5, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

atomic_int x = 0;
atomic_int y = 0;

void *
t1()
{
    x++;
    x++;
    assert(x != 0b11);
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
    y++;
    y++;
    assert(y != 0b11);
    return NULL;
}
void *
t4()
{
    y *= 2;
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

    return 0;
}

// b1:               s_id t_id
// x ++ -> x *= 2       2 3
// x *= 2 -> x ++       3 2

// b2:
// y ++ -> y *= 2       4 5
// y *= 2 -> y ++       5 4

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

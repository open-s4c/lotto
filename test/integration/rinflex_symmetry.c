// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | %check %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AWRITE
// CHECK-NEXT: [thread]
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AREAD
// CHECK-NEXT: [thread]

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AWRITE
// CHECK-NEXT: [thread]
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AREAD
// CHECK-NEXT: [thread]

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AREAD
// CHECK-NEXT: [thread,get_unique_id]
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AWRITE
// CHECK-NEXT: [thread,get_unique_id]

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AREAD
// CHECK-NEXT: [thread,get_unique_id]
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_AWRITE
// CHECK-NEXT: [thread,get_unique_id]

// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

atomic_int x        = 0;
atomic_int array[2] = {0, 0};

int
get_unique_id()
{
    /* R1 -> W2
       R2 -> W1 */
    int y = x;     /* R */
    x     = y + 1; /* W */
    return y;
}

void *
thread(void *arg)
{
    /* Write1 -> Assert2
       Write2 -> Assert1 */
    int value = (int)(intptr_t)arg;
    int id    = get_unique_id();
    array[id] = value;          /* Write */
    assert(array[id] == value); /* Assert */
    return NULL;
}

int
main()
{
    pthread_t t1_t, t2_t;

    pthread_create(&t1_t, 0, thread, (void *)1);
    pthread_create(&t2_t, 0, thread, (void *)2);

    pthread_join(t1_t, 0);
    pthread_join(t2_t, 0);

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

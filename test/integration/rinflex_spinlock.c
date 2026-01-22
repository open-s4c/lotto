// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %rinflex-stress -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | %check %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_XCHG
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_XCHG

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_XCHG
// CHECK: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_XCHG

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: AFTER_CMPXCHG_S

// clang-format on

#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <sched.h>

#include <lotto/order.h>

typedef atomic_flag spinlock_t;
#define SPINLOCK_INIT ATOMIC_FLAG_INIT
#define LOCK(l) while(atomic_flag_test_and_set((l)))
#define UNLOCK(l) atomic_flag_clear((l))

spinlock_t s = SPINLOCK_INIT;
int x = 0;

void
lock(spinlock_t *s)
{
    while(atomic_flag_test_and_set(s));
}

void
unlock(spinlock_t *s)
{
    atomic_flag_clear(s);
}

void *
func1(void *arg)
{
    lock(&s);
    x += 1;
    unlock(&s);
    lock(&s);
    x += 1;
    unlock(&s);
    return 0;
}

void *
func2(void *arg)
{
    lock(&s);
    x *= 2;
    unlock(&s);
    lock(&s);
    assert(x != 0b11);
    unlock(&s);
    return 0;
}

int
main()
{
    pthread_t t1, t2;
    pthread_create(&t1, 0, func1, 0);
    pthread_create(&t2, 0, func2, 0);
    pthread_join(t1, 0);
    pthread_join(t2, 0);
    return 0;
}

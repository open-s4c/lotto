// clang-format off
// ALLOW_RETRIES: 5
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress4rinflex -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | %check %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK-NEXT: event - tid: 2, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW
// CHECK: event - tid: 3, clk: {{[0-9]+}}, {{[0-9]+}} x pc: {{.*}}, cat: BEFORE_RMW

// clang-format on

#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

typedef atomic_int spinlock_t;
atomic_int _chpt;

void lock(spinlock_t *lock) {
    while (atomic_fetch_or(lock, 1) & 1);
}

void unlock(spinlock_t *lock) {
    atomic_store(lock, 0);
}

spinlock_t l;
int x;

void *
func1(void *arg)
{
    lock(&l);
    x += 1;
    unlock(&l);
    lock(&l);
    x += 1;
    unlock(&l);
    return 0;
}

void *
func2(void *arg)
{
    lock(&l);
    x *= 2;
    unlock(&l);
    lock(&l);
    assert(x != 0b11);
    unlock(&l);
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


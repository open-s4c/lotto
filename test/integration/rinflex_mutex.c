// clang-format off
// REQUIRES: STABLE_ADDRESS_MAP, RUST_HANDLERS_AVAILABLE
// UNSUPPORTED: aarch64
// RUN: (! %lotto %rinflex-stress -- %b)
// RUN: %lotto %rinflex -r 30 2>&1 | iconv -t utf-8 -f utf-8 -c | filecheck %s

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: MUTEX_ACQUIRE
// CHECK-NEXT: [func{{[12]}}]
// CHECK: event - tid: 4, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: MUTEX_ACQUIRE
// CHECK-NEXT: [func3]

// CHECK: ------ . ------ . ------ . ------ . ------
// CHECK: event - tid: {{[23]}}, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: MUTEX_ACQUIRE
// CHECK-NEXT: [func{{[12]}}]
// CHECK: event - tid: 4, clk: {{[0-9]+}}, 1 x pc: {{.*}}, cat: MUTEX_ACQUIRE
// CHECK-NEXT: [func3]

// clang-format on

#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int x = 0;

void *
func1(void *arg)
{
    pthread_mutex_lock(&m);
    x += 1;
    pthread_mutex_unlock(&m);
    return 0;
}

void *
func2(void *arg)
{
    pthread_mutex_lock(&m);
    x += 2;
    pthread_mutex_unlock(&m);
    return 0;
}

void *
func3(void *arg)
{
    pthread_mutex_lock(&m);
    assert(x < 3);
    pthread_mutex_unlock(&m);
    return 0;
}

int
main()
{
    pthread_t t1, t2, t3;
    pthread_create(&t1, 0, func1, 0);
    pthread_create(&t2, 0, func2, 0);
    pthread_create(&t3, 0, func3, 0);
    pthread_join(t1, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

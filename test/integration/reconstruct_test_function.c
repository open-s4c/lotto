// clang-format off
// UNSUPPORTED: aarch64
// XFAIL: *
// RUN: exit 1
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_function_perfect.log --reconstruct-rounds 5 -- %b 2>&1) | %check %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_function_backtrack.log --reconstruct-rounds 5 -- %b 2>&1) | %check %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_function_perfect.log --reconstruct-strategy random --reconstruct-rounds 5 -- %b 2>&1) | %check %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_function_backtrack.log --reconstruct-strategy random --reconstruct-rounds 5 -- %b 2>&1) | %check %s
// CHECK: [1] assert failed {{.*}}/reconstruct_test_function.c:{{[0-9]+}}: x != 3
// CHECK: [lotto] Trace reconstructed successfully!
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/log.h>

atomic_int x;

void *
run()
{
    lotto_log(1);
    atomic_fetch_add(&x, 1);
    lotto_log(2);
    return NULL;
}

int
main()
{
    pthread_t t2, t3, t4;
    atomic_init(&x, 0);
    pthread_create(&t2, 0, run, 0);
    pthread_create(&t3, 0, run, 0);
    pthread_create(&t4, 0, run, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);
    pthread_join(t4, 0);
    assert(x != 3);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

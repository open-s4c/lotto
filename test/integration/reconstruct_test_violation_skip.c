// clang-format off
// UNSUPPORTED: aarch64
// RUN: %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_violation_perfect.log --reconstruct-no-violation-search --reconstruct-rounds 5 -- %b 2>&1 | %check %s
// RUN: %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_violation_backtrack.log --reconstruct-no-violation-search --reconstruct-rounds 5 -- %b 2>&1 | %check %s
// RUN: %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_violation_perfect.log --reconstruct-strategy random --reconstruct-no-violation-search --reconstruct-rounds 5 -- %b 2>&1 | %check %s
// RUN: %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_violation_backtrack.log --reconstruct-strategy random --reconstruct-no-violation-search --reconstruct-rounds 5 -- %b 2>&1 | %check %s
// CHECK: [lotto] Trace reconstructed successfully!
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/log.h>

atomic_int x;

void *
thread2()
{
    lotto_log(1);
    atomic_fetch_add(&x, 1);
    atomic_fetch_add(&x, 1);
    lotto_log(2);
    return NULL;
}

void *
thread3()
{
    lotto_log(1);
    int a = atomic_load(&x);
    lotto_log(2);
    assert(a != -1);
    return NULL;
}

int
main()
{
    pthread_t t2, t3;
    atomic_init(&x, 0);
    pthread_create(&t2, 0, thread2, 0);
    pthread_create(&t3, 0, thread3, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

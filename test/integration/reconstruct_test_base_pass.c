// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_base_perfect.log --reconstruct-rounds 5 -- %b 2>&1) | filecheck %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_base_backtrack.log --reconstruct-rounds 5 -- %b 2>&1) | filecheck %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_base_perfect.log --reconstruct-strategy random --reconstruct-rounds 5 -- %b 2>&1) | filecheck %s
// RUN: (! %lotto %reconstruct --reconstruct-log %S/log/reconstruct_test_base_backtrack.log --reconstruct-strategy random --reconstruct-rounds 5 -- %b 2>&1) | filecheck %s
// CHECK: [1] assert failed {{.*}}/reconstruct_test_base_pass.c:{{[0-9]+}}: a != 4
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
    lotto_log(2);
    lotto_log(1);
    atomic_fetch_add(&x, 2);
    lotto_log(2);
    return NULL;
}

void *
thread3()
{
    lotto_log(1);
    int a = atomic_fetch_xor(&x, 7);
    lotto_log_data(2, a);
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
    int a = atomic_load(&x);
    assert(a != 4);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

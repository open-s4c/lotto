// clang-format off
// CHECK: assert failed {{.*}}/RaRmw-5_RaRmw-5.{{.+}}.c:{{[0-9]+}}: a != 55 && b != 89
// CHECK: [lotto] Trace reconstructed successfully!
// RUN: (! %lotto reconstruct --reconstruct-log %logfile -- %b 2>&1) | filecheck %s
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include <lotto/log.h>

atomic_int x;
atomic_int y;

void *
thread2()
{
    for (int i = 0; i < 5; i++) {
        lotto_log(1);
        int a = atomic_load(&y);
        lotto_log(2);
        atomic_fetch_add(&x, a);
        lotto_log(3);
    }
    return NULL;
}

void *
thread3()
{
    for (int i = 0; i < 5; i++) {
        lotto_log(1);
        int a = atomic_load(&x);
        lotto_log(2);
        atomic_fetch_add(&y, a);
        lotto_log(3);
    }
    return NULL;
}

int
main()
{
    atomic_init(&x, 0);
    atomic_init(&y, 1);

    pthread_t t2, t3;
    pthread_create(&t2, 0, thread2, 0);
    pthread_create(&t3, 0, thread3, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);

    int a = atomic_load(&x);
    int b = atomic_load(&y);
    lotto_log_data(3, a);
    lotto_log_data(4, b);
    assert(a != 55 && b != 89);

    return 0;
}

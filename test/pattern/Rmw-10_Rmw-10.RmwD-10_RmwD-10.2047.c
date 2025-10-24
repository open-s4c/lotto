// clang-format off
// CHECK: assert failed {{.*}}/Rmw-10_Rmw-10.{{.+}}.c:{{[0-9]+}}: a != 2047
// CHECK: [lotto] Trace reconstructed successfully!
// RUN: (! %lotto reconstruct --reconstruct-log %logfile -- %b 2>&1) | filecheck %s
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include <lotto/log.h>

atomic_int x;

void *
thread2()
{
    for (int i = 0; i < 10; i++) {
        int a = atomic_fetch_add(&x, 1);
        lotto_log_data(2, a);
    }
    return NULL;
}

void *
thread3()
{
    for (int i = 0; i < 10; i++) {
        int a = atomic_fetch_xor(&x, (2 << i) - 1);
        lotto_log_data(2, a);
    }
    return NULL;
}

int
main()
{
    atomic_init(&x, 1);

    pthread_t t2, t3;
    pthread_create(&t2, 0, thread2, 0);
    pthread_create(&t3, 0, thread3, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);

    int a = atomic_load(&x);
    lotto_log_data(3, a);
    assert(a != 2047);

    return 0;
}

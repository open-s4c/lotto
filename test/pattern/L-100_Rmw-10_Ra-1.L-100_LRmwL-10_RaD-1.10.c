// clang-format off
// CHECK: assert failed {{.*}}/L-100_Rmw-10_Ra-1.{{.+}}.c:{{[0-9]+}}: a != 10
// CHECK: [lotto] Trace reconstructed successfully!
// RUN: (! %lotto reconstruct --reconstruct-log %logfile -- %b 2>&1) | filecheck %s
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/log.h>

atomic_int x;

void *
thread2()
{
    for (int i = 0; i < 100; i++) {
        lotto_log(i);
    }
    return NULL;
}

void *
thread3()
{
    for (int i = 0; i < 10; i++) {
        lotto_log(1);
        atomic_fetch_add(&x, 1);
        lotto_log(2);
    }
    return NULL;
}

void *
thread4()
{
    int a = atomic_load(&x);
    lotto_log_data(1, a);
    assert(a != 10);
    return NULL;
}

int
main()
{
    atomic_init(&x, 0);

    pthread_t t2, t3, t4;
    pthread_create(&t2, 0, thread2, 0);
    pthread_create(&t3, 0, thread3, 0);
    pthread_create(&t4, 0, thread4, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);
    pthread_join(t4, 0);

    return 0;
}

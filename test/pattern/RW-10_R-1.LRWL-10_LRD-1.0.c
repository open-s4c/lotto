// clang-format off
// UNSUPPORTED: aarch64
// CHECK: assert failed {{.*}}/RW-10_R-1.{{.+}}.c:{{[0-9]+}}: a != 0
// CHECK: [lotto] Trace reconstructed successfully!
// RUN: (! %lotto reconstruct --reconstruct-log %logfile --reconstruct-nonatomic -- %b 2>&1) | filecheck %s
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#include <lotto/log.h>

int x = 0;

void *
thread2()
{
    for (int i = 0; i < 10; i++) {
        lotto_log(1);
        x++;
        lotto_log(2);
    }
    return NULL;
}

void *
thread3()
{
    lotto_log(1);
    int a = x;
    lotto_log_data(2, a);
    assert(a != 0);
    return NULL;
}

int
main()
{
    pthread_t t2, t3;
    pthread_create(&t2, 0, thread2, 0);
    pthread_create(&t3, 0, thread3, 0);
    pthread_join(t2, 0);
    pthread_join(t3, 0);
    return 0;
}

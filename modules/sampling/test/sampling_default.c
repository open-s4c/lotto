// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/sampling_default.c:{{[0-9]+}}: x != 1
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *
thread1(void *arg)
{
    (void)arg;
    x++;
    x++;
    return 0;
}

void *
thread2(void *arg)
{
    (void)arg;
    assert(x != 1);
    return 0;
}

int
main()
{
    pthread_t t1, t2;
    pthread_create(&t1, 0, thread1, 0);
    pthread_create(&t2, 0, thread2, 0);
    pthread_join(t2, 0);
    pthread_join(t1, 0);
    return 0;
}

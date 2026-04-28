// clang-format off
// RUN: (! %lotto %stress -a MASK -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/atomic_return.c:{{[0-9]+}}: x == old_x
// clang-format on

#include <assert.h>
#include <pthread.h>

#include <lotto/region_atomic.h>

int x;

int
f()
{
    lotto_atomic({
        x++;
        return x;
    });
    return 0;
}

static void *
thread(void *arg)
{
    (void)arg;
    int old_x = f();
    assert(x == old_x);
    return NULL;
}


int
main()
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, 0, thread, 0);
    pthread_create(&thread2, 0, thread, 0);
    pthread_join(thread1, 0);
    pthread_join(thread2, 0);
    return 0;
}

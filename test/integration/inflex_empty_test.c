// clang-format off
// RUN: (! %lotto %stress -r 1 -- %b)
// RUN: %lotto %inflex -r 1 -s random -- %b
// RUN: %lotto %show -- %b 2>&1 | %check
// CHECK: clock:{{ *0}}
// CHECK-COUNT: 1 clock:
// clang-format on

#include <pthread.h>

static void *
noop(void *arg)
{
    (void)arg;
    return NULL;
}

int
main(void)
{
    pthread_t thread1;
    pthread_t thread2;

    pthread_create(&thread1, NULL, noop, NULL);
    pthread_create(&thread2, NULL, noop, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}

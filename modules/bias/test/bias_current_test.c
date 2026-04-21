// clang-format off
// RUN: (! %lotto %stress -s random -r 20 --region-preemption=off --bias-policy=CURRENT -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/bias_current_test.c:{{[0-9]+}}: x != val
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

static volatile int go;
static volatile int ready;
static int x;

static void *
run_thread(void *arg)
{
    (void)arg;

    ready++;
    while (!go)
        ;
    int val = x;
    sched_yield();
    assert(x != val);
    return NULL;
}

int
main(void)
{
    pthread_t alice, bob;

    pthread_create(&alice, NULL, run_thread, NULL);
    pthread_create(&bob, NULL, run_thread, NULL);
    while (ready != 2)
        ;
    go = 1;
    pthread_join(alice, NULL);
    pthread_join(bob, NULL);
    return 0;
}

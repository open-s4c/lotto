// clang-format off
// RUN: (! %lotto %stress -d pos -r 20 --region-preemption=off --bias-policy=LOWEST -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/bias_lowest_test.c:{{[0-9]+}}: val == 0
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

static volatile int go;
static volatile int ready;
static volatile int stage;

static void *
run_alice(void *arg)
{
    (void)arg;

    ready++;
    while (!go)
        ;
    sched_yield();
    stage = 1;
    return NULL;
}

static void *
run_bob(void *arg)
{
    (void)arg;

    ready++;
    while (!go)
        ;
    sched_yield();
    int val = stage;
    assert(val == 0);
    return NULL;
}

int
main(void)
{
    pthread_t alice, bob;

    pthread_create(&alice, NULL, run_alice, NULL);
    pthread_create(&bob, NULL, run_bob, NULL);
    while (ready != 2)
        ;
    go = 1;
    pthread_join(alice, NULL);
    pthread_join(bob, NULL);
    return 0;
}

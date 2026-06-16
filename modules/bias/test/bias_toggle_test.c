// clang-format off
// RUN: (! %lotto %stress -d pos -r 20 --region-preemption=off --bias-policy=LOWEST --bias-toggle=HIGHEST -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/bias_toggle_test.c:{{[0-9]+}}: val == 1
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

#include <lotto/bias.h>

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
    assert(val == 1);
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
    lotto_bias_toggle();
    go = 1;
    pthread_join(alice, NULL);
    pthread_join(bob, NULL);
    return 0;
}

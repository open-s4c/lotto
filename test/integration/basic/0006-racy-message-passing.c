// REQUIRES: dice-runtime
// RUN: (! %lotto %stress -- %b) 2>&1 | %check %s
// CHECK: assert failed {{.*}} val == 1

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>

static volatile int g_ctrl;
static volatile int g_data;

static void *
run_alice(void *arg)
{
    (void)arg;
    g_ctrl = 1;
    g_data = 1;
    return NULL;
}

static void *
run_bob(void *arg)
{
    (void)arg;
    while (!g_ctrl) {}

    int val = g_data;
    assert(val == 1);

    return NULL;
}

int
main(void)
{
    pthread_t alice;
    pthread_t bob;

    g_ctrl = 0;
    g_data = 0;

    pthread_create(&bob, NULL, run_bob, NULL);
    pthread_create(&alice, NULL, run_alice, NULL);

    void *bob_status;
    pthread_join(alice, NULL);
    pthread_join(bob, &bob_status);

    return bob_status == NULL ? 0 : 1;
}

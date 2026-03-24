// clang-format off
// REQUIRES: DISABLED
// Loops not yet supported for rinflex.
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <sched.h>

int x = 0;

void *
run_alice(void *arg)
{
    for (int a = 0; a != 50; a++) {
        x++;
    }
    return NULL;
}

void *
run_bob(void *arg)
{
    assert(x % 10 == 0 && x != 0);
    return NULL;
}

int
main()
{
    pthread_t alice, bob;
    pthread_create(&bob, 0, run_bob, 0);
    pthread_create(&alice, 0, run_alice, 0);
    pthread_join(alice, 0);
    pthread_join(bob, 0);
    return 0;
}


// clang-format off
// NOTE: Original lit script executed `RUN: exit 1` here
// RUN: echo Loops not yet supported for rinflex
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
    pthread_t t1;
    pthread_t t2;
    pthread_create(&t1, NULL, noop, NULL);
    pthread_create(&t2, NULL, noop, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}

// clang-format off
// UNSUPPORTED: aarch64
// XFAIL: *
// NOTE: Original lit script executed `RUN: exit 1` here
// ALLOW_RETRIES: 100
// RUN: (! %lotto %stress -v -s random --check-deadlock -- %b)
// RUN: env LOTTO_MOCK_TEST=inflex_deadlock_test %lotto %inflex -s random --handler-lock enable -r 100
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

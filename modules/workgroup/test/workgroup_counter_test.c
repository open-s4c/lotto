// clang-format off
// RUN: %lotto %stress -r 5 --workgroup-thread-start-policy CONCURRENT -- %b | %check %s
// CHECK: WORKGROUP_LOG counter=4
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include <lotto/workgroup.h>
#include <vsync/atomic.h>

enum {
    NTHREADS = 4,
};

static vatomic32_t started;
static volatile int counter;

static void *
worker_(void *arg)
{
    (void)arg;
    vatomic_inc(&started);
    lotto_workgroup_join();
    counter++;
    return NULL;
}

int
main(void)
{
    pthread_t threads[NTHREADS];

    setbuf(stdout, NULL);

    for (int i = 0; i < NTHREADS; i++) {
        assert(pthread_create(&threads[i], NULL, worker_, NULL) == 0);
    }

    vatomic_await_eq(&started, NTHREADS);
    lotto_workgroup_join();

    for (int i = 0; i < NTHREADS; i++) {
        assert(pthread_join(threads[i], NULL) == 0);
    }

    printf("WORKGROUP_LOG counter=%d\n", counter);
    assert(counter == NTHREADS);
    return 0;
}

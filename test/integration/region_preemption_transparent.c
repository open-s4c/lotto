// clang-format off
// RUN: (! %lotto %stress -- %b 2>&1) | filecheck %s
// CHECK: assert failed {{.*}}/region_preemption_transparent.c:{{[0-9]+}}: a == 1
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/region_atomic.h>

int x;

void
init_test()
{
    x = 0;
}

void *
t1(void *_)
{
    __atomic_store_n(&x, 1, __ATOMIC_SEQ_CST);
    lotto_atomic({
        int a = __atomic_load_n(&x, __ATOMIC_SEQ_CST);
        assert(a == 1);
    });
    return 0;
}

void *
t2(void *_)
{
    __atomic_store_n(&x, 2, __ATOMIC_SEQ_CST);
    return 0;
}

pthread_t thr[2];

void
lotto_block_test()
{
    init_test();
    pthread_create(&thr[0], NULL, t1, NULL);
    pthread_create(&thr[1], NULL, t2, NULL);

    for (int i = 0; i < 2; i++)
        pthread_join(thr[i], NULL);
}

int
main()
{
    lotto_block_test();
    return 0;
}

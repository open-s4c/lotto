// clang-format off
// RUN: (! %lotto %stress -r 10 -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/producer_consumer.c:{{[0-9]+}}: d != NULL && "no data?"
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define K 1000
#define M 1001

typedef struct {
    int content;
} data_t;

void consume_data(data_t *d);
data_t *produce_data(int cnt);

atomic_int nxt;
data_t *data[M];

void *
sender()
{
    for (int cnt = 0; cnt <= K; cnt++) {
        data_t *d = produce_data(cnt);
        nxt       = cnt;
        data[nxt] = d;
    }
    return 0;
}

void *
receiver()
{
    for (int last = nxt; last < K; last = nxt) {
        data_t *d = data[last];
        consume_data(d);
    }
    return 0;
}


void
consume_data(data_t *d)
{
    assert(d != NULL && "no data?");
}

data_t *
produce_data(int cnt)
{
    (void)cnt;
    data_t *d = (data_t *)malloc(sizeof(data_t));
    return d;
}

int
main()
{
    pthread_t t1, t2;
    pthread_create(&t1, 0, sender, 0);
    pthread_create(&t2, 0, receiver, 0);
    pthread_join(t1, 0);
    pthread_join(t2, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: (! %lotto %stress -r 10 -- %b 2>&1) | %check %s
// CHECK: assert failed {{.*}}/queue_mpmc.c:{{[0-9]+}}: d->content != 2
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <vsync/queue/bounded_mpmc.h>

#define QUEUE_SIZE 5
bounded_mpmc_t q;
vatomic32_t first;

typedef struct {
    int content;
} data_t;


void *
sender()
{
    data_t *d  = (data_t *)malloc(sizeof(data_t));
    d->content = 1;

    int err = bounded_mpmc_enq(&q, d);
    assert(err == QUEUE_BOUNDED_OK);

    d          = (data_t *)malloc(sizeof(data_t));
    d->content = 2;

    err = bounded_mpmc_enq(&q, d);
    assert(err == QUEUE_BOUNDED_OK);

    return 0;
}

void *
receiver()
{
    bool check = vatomic32_get_inc(&first) != 0;

    void *p;
    data_t *d;
    int err;

    while ((err = bounded_mpmc_deq(&q, &p)) != QUEUE_BOUNDED_EMPTY) {
        if (err == QUEUE_BOUNDED_AGAIN)
            continue;
        d = (data_t *)p;
        if (check)
            assert(d->content != 2);
        free(d);
    }
    return 0;
}

int
main()
{
    void *buf[QUEUE_SIZE];
    bounded_mpmc_init(&q, buf, QUEUE_SIZE);

    pthread_t t1, t2;
    sender();

    pthread_create(&t1, 0, receiver, 0);
    pthread_create(&t2, 0, receiver, 0);
    pthread_join(t1, 0);
    pthread_join(t2, 0);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

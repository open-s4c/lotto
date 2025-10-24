// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: echo Disable ASLR and remove lines 2-4 to make this test pass
// RUN: %lotto %sc --delayed-functions quack_push:1:1,quack_popall:1:1 --delayed-calls 2 --sc-exploration-rounds 100 -r 100 --stable-address-method MAP -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include "quack.h"

#define NTHREADS 2
#define NODES    3

quack_t quack;

typedef struct {
    quack_node_t n;
    int v;
} node_t;

node_t nodes[NODES];
vatomic32_t ci;
vatomic32_t co;

static void *
producer(void *arg)
{
    (void)arg;
    int i;
    while ((i = vatomic32_get_inc_rlx(&ci)) < NODES) {
        node_t *n = &nodes[i];
        n->v      = i;
        quack_push(&quack, &n->n);
    }
    return 0;
}

static void *
consumer(void *arg)
{
    (void)arg;
    await_do {
        quack_node_t *n = quack_reverse(quack_popall(&quack));
        for (; n != NULL; n = n->next) {
            node_t *nn = (node_t *)n;
            nn->v      = -1;
            vatomic32_inc_rlx(&co);
        }
    }
    while_await(vatomic32_read_rlx(&co) < NODES);
    assert(vatomic32_read_rlx(&co) == NODES);
    return 0;
}

void *
run(void *arg)
{
    vuintptr_t tid = (vuintptr_t)arg;
    if (tid < NTHREADS / 2)
        return producer(NULL);
    else
        return consumer(NULL);
}

int
main()
{
    pthread_t t[NTHREADS];
    for (int i = 0; i < NTHREADS; i++)
        pthread_create(&t[i], 0, run, (void *)(vuintptr_t)i);
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(t[i], 0);

    for (int i = 0; i < NODES; i++)
        assert(nodes[i].v == -1);
    assert(vatomic32_read_rlx(&ci) == NODES + (NTHREADS / 2));
    assert(vatomic32_read_rlx(&co) == NODES);
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

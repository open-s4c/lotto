// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: echo Disable ASLR and remove lines 2-4 to make this test pass
// RUN: (! %lotto %sc --delayed-functions quack_push:1:1,quack_pop:1:1 --delayed-calls 4 --sc-exploration-rounds 100 -- %b 2>&1) | %check %s
// CHECK-NOT: [lotto] no atomic execution satisfies the nonatomic partial order, try more --sc-exploration-rounds
// CHECK: assert failed {{.*}}/handler_capture_group.c:{{[0-9]+}}: 0 && "could not find a corresponding sequential state"
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#include "quack.h"

#define NTHREADS 4
#define NODES    10

quack_t quack;

typedef struct {
    quack_node_t n;
    int v;
} node_t;

node_t nodes[NODES];
vatomic32_t ci;
vatomic32_t co;

quack_node_t *
quack_pop(quack_t *q)
{
    quack_node_t *head = quack_popall(q);
    if (head && head->next) {
        quack_push(q, head->next);
        head->next = NULL;
    }
    return head;
}

static void *
producer(void *arg)
{
    (void)arg;
    int i;
    while ((i = vatomic32_get_inc_rlx(&ci)) < NODES) {
        node_t *n = &nodes[i];
        n->v      = i + 1;
        quack_push(&quack, &n->n);
        vatomic32_inc_rlx(&co);
    }
    return 0;
}

static void *
consumer(void *arg)
{
    (void)arg;
    await_do {
        node_t *n = (node_t *)quack_pop(&quack);
        assert(!n || n->v);
    }
    while_await(vatomic32_read_rlx(&co) < NODES && !quack_is_empty(&quack));
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
    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

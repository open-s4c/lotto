// clang-format off
// XFAIL: *
// RUN: exit 1
// RUN: echo Disable ASLR and remove lines 2-4 to make this test pass
// RUN: %lotto %sc --delayed-functions enqueue:0:0,dequeue:1:0 --delayed-calls 3 --sc-exploration-rounds 500 -r 500 -- %b
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define N 10
_Atomic(int *) buf[N];
atomic_size_t len;

void
enqueue(int *data)
{
    buf[len++] = data;
}

int *
dequeue()
{
#ifdef BUG
    #define THE_LEN_EXPR (len)
#else
    #define THE_LEN_EXPR (curLen)
#endif
    while (true) {
        size_t curLen = len;
        if (curLen >= N) {
            return NULL;
        }
        for (size_t i = 0; i < THE_LEN_EXPR; i++) {
            int *res;
            if ((res = atomic_exchange(&buf[i], NULL))) {
                return res;
            }
        }
    }
#undef THE_LEN_EXPR
}

void *
treader()
{
    dequeue();
    return NULL;
}

void *
twriter(void *x)
{
    int *e = malloc(sizeof(int));
    *e     = *(int *)x;
    enqueue(e);
    return NULL;
}

int
main()
{
    pthread_t reader, writer1, writer2;
    int one = 1, two = 2;
    pthread_create(&reader, 0, treader, 0);
    pthread_create(&writer1, 0, twriter, &one);
    pthread_create(&writer2, 0, twriter, &two);
    pthread_join(reader, 0);
    pthread_join(writer1, 0);
    pthread_join(writer2, 0);

    return 0;
}

#include "lotto/qemu/lotto_qemu_sighandler.h"
QLOTTO_SIGHANDLER
#include "lotto/qemu/lotto_qemu_deconstructor.h"
QLOTTO_DECONSTRUCTOR

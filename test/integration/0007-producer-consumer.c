// RUN: %lotto %record -- %b | grep TEST_LOG > %b.record
// RUN: %lotto %replay | grep TEST_LOG > %b.replay
// RUN: diff %b.record %b.replay

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define K 64

typedef struct {
    int value;
} data_t;

static data_t *g_data[K + 1];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond   = PTHREAD_COND_INITIALIZER;
static int g_next;
static int g_done;

static data_t *
produce_data(int value)
{
    data_t *d = (data_t *)malloc(sizeof(*d));
    if (d != NULL) {
        d->value = value;
    }
    return d;
}

static void
consume_data(data_t *d)
{
    free(d);
}

static void *
sender(void *arg)
{
    (void)arg;
    for (int i = 0; i < K; ++i) {
        data_t *d = produce_data(i);

        pthread_mutex_lock(&g_mutex);
        g_data[i] = d;
        g_next    = i;
        printf("TEST_LOG PRODUCE %d\n", i);
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_mutex);
    }

    pthread_mutex_lock(&g_mutex);
    g_done = 1;
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mutex);
    return NULL;
}

static void *
receiver(void *arg)
{
    (void)arg;
    int idx = 0;

    while (1) {
        pthread_mutex_lock(&g_mutex);
        while (!g_done && (idx > g_next || g_data[idx] == NULL)) {
            pthread_cond_wait(&g_cond, &g_mutex);
        }

        if (idx <= g_next && g_data[idx] != NULL) {
            data_t *d   = g_data[idx];
            g_data[idx] = NULL;
            pthread_mutex_unlock(&g_mutex);
            consume_data(d);
            printf("TEST_LOG CONSUME %d\n", idx);
            ++idx;
        } else if (g_done) {
            pthread_mutex_unlock(&g_mutex);
            break;
        } else {
            pthread_mutex_unlock(&g_mutex);
        }
    }

    return NULL;
}

#define NTHREADS 8

int
main(void)
{
    pthread_t t[NTHREADS];

    for (int i = 0; i < NTHREADS / 2; i++)
        pthread_create(&t[i], NULL, sender, NULL);

    for (int i = NTHREADS / 2; i < NTHREADS; i++)
        pthread_create(&t[i], NULL, receiver, NULL);

    for (int i = 0; i < NTHREADS; i++)
        pthread_join(t[i], NULL);

    return 0;
}

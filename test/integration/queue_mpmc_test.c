// XFAIL: *
// NOTE: Original lit script executed `RUN: exit 1` here
// RUN: (! %lotto %stress -r 10 -- %b 2>&1) | %check
// CHECK: assert failed: d->content != 2

#include <pthread.h>
#include <stdbool.h>

#define QUEUE_CAP 4
#define SENTINEL -1

typedef struct {
    int buffer[QUEUE_CAP];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

static queue_t g_queue;
static pthread_mutex_t g_first_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_seen_value_two;

static void
queue_init(queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

static void
queue_push(queue_t *q, int value)
{
    pthread_mutex_lock(&q->mutex);
    while (q->count == QUEUE_CAP) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    q->buffer[q->tail] = value;
    q->tail = (q->tail + 1) % QUEUE_CAP;
    ++q->count;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

static int
queue_pop(queue_t *q, int *value)
{
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    *value = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    --q->count;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

static void *
sender(void *arg)
{
    (void)arg;
    queue_push(&g_queue, 1);
    queue_push(&g_queue, 2);
    queue_push(&g_queue, SENTINEL);
    queue_push(&g_queue, SENTINEL);
    return NULL;
}

static void *
receiver(void *arg)
{
    (void)arg;
    while (1) {
        int value;
        queue_pop(&g_queue, &value);
        if (value == SENTINEL) {
            break;
        }
        if (value == 2) {
            pthread_mutex_lock(&g_first_mutex);
            bool already_seen = g_seen_value_two;
            if (!already_seen) {
                g_seen_value_two = true;
            }
            pthread_mutex_unlock(&g_first_mutex);
            if (already_seen) {
                return (void *)1;
            }
        }
    }
    return NULL;
}

int
main(void)
{
    queue_init(&g_queue);

    pthread_t producer;
    pthread_t consumer_a;
    pthread_t consumer_b;

    pthread_create(&producer, NULL, sender, NULL);
    pthread_create(&consumer_a, NULL, receiver, NULL);
    pthread_create(&consumer_b, NULL, receiver, NULL);

    pthread_join(producer, NULL);
    void *result_a;
    void *result_b;
    pthread_join(consumer_a, &result_a);
    pthread_join(consumer_b, &result_b);

    return (result_a == NULL && result_b == NULL) ? 0 : 1;
}

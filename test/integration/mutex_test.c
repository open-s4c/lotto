// RUN: %lotto %stress -r 50 -- %b

#include <pthread.h>

#define N_THREADS 100

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
thread(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&g_mutex);
    pthread_mutex_unlock(&g_mutex);
    return NULL;
}

int
main(void)
{
    pthread_t threads[N_THREADS];

    for (int i = 0; i < N_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, thread, NULL) != 0) {
            return 1;
        }
    }

    for (int i = 0; i < N_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

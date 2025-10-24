#include <assert.h>
#include <pthread.h>
#define NTHREADS 10
int x;
pthread_mutex_t m;

void *
run(void *_)
{
    pthread_mutex_lock(&m);
    x++;
    pthread_mutex_unlock(&m);
    return 0;
}

int
main()
{
    pthread_t t[NTHREADS];
    for (int i = 0; i < NTHREADS; i++)
        pthread_create(&t[i], 0, run, 0);
    run(0);
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(t[i], 0);
    assert(x == NTHREADS + 1);
    return 0;
}

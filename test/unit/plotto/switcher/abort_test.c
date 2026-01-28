#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "lotto/base/cappt.h"
#include <lotto/base/context.h>
#include <lotto/runtime/switcher.h>
#include <lotto/sys/ensure.h>

int counter = 0;

void *
r1()
{
    int tid = 2;
    int yd  = switcher_yield(tid, NULL);
    if (yd == SWITCHER_ABORTED)
        return 0;
    switcher_abort();
    switcher_wake(ANY_TASK, 0);
    ++counter;
    return 0;
}


void *
r2()
{
    int tid = 3;
    int yd  = switcher_yield(tid, NULL);
    if (yd == SWITCHER_ABORTED)
        return 0;
    switcher_abort();
    switcher_wake(ANY_TASK, 0);
    ++counter;
    return 0;
}

int
main()
{
    pthread_t t[2];

    pthread_create(&t[0], 0, r1, 0);
    pthread_create(&t[1], 0, r2, 0);

    switcher_wake(ANY_TASK, 0);


    pthread_join(t[0], 0);
    pthread_join(t[1], 0);

    printf("%d\n", counter);
    ENSURE(counter == 1);
}

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include <lotto/base/context.h>
#include <lotto/runtime/switcher.h>
#include <lotto/sys/ensure.h>

int counter                = 0;
int pid                    = 0;
bool asserted_condition_r3 = false;

void *
r1()
{
    int yd = switcher_yield(2, NULL);
    if (yd == SWITCHER_ABORTED)
        return 0;
    ++counter;
    switcher_wake(ANY_TASK, 0);
    return 0;
}


void *
r2()
{
    int yd = switcher_yield(3, NULL);
    if (yd == SWITCHER_ABORTED)
        return 0;
    ++counter;
    switcher_wake(ANY_TASK, 0);
    return 0;
}

void *
r3()
{
    int yd = switcher_yield(4, NULL);
    if (yd == SWITCHER_ABORTED)
        return 0;
    asserted_condition_r3 = true;
    ++counter;
    switcher_wake(5, 0);
    return 0;
}

static bool
_true(task_id id)
{
    return true;
}

void *
r4()
{
    int yd = switcher_yield(5, _true);
    if (yd == SWITCHER_ABORTED)
        return 0;
    ++counter;
    switcher_wake(ANY_TASK, 0);
    return 0;
}


int
main()
{
    pthread_t t[4];

    pthread_create(&t[0], 0, r1, 0);
    pthread_create(&t[1], 0, r2, 0);
    pthread_create(&t[2], 0, r3, 0);
    pthread_create(&t[3], 0, r4, 0);


    switcher_wake(ANY_TASK, 0);


    pthread_join(t[0], 0);
    pthread_join(t[1], 0);
    pthread_join(t[2], 0);
    pthread_join(t[3], 0);

    ENSURE(counter == 4);
}

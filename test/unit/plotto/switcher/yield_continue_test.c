#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include <lotto/base/context.h>
#include <lotto/runtime/switcher.h>


int
mock_get_task_id()
{
    return 2;
}

void *
r1()
{
    int tid = mock_get_task_id();
    int yd  = switcher_yield(tid, NO_ANY_TASK_FILTERS);
    if (yd == SWITCHER_ABORTED)
        return 0;
    switcher_wake(2, 0);
    yd = switcher_yield(tid, NO_ANY_TASK_FILTERS);
    printf("condition true %d\n", yd == SWITCHER_CONTINUE);
    assert(yd == SWITCHER_CONTINUE);
    return 0;
}


int
main()
{
    pthread_t t;

    pthread_create(&t, 0, r1, 0);

    switcher_wake(2, 0);

    pthread_join(t, 0);
}

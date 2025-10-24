#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <lotto/base/context.h>
#include <lotto/runtime/switcher.h>
#include <lotto/sys/ensure.h>

int counter = 0;
int pid     = 0;

time_t start, finish;


void *
r1()
{
    sleep(2); // Forces yield of r2 to execute while the next is still r1
              // thread, running the nanosleep.
    printf("Entered thread 1\n");
    int yd = switcher_yield(2, false);
    if (yd == SWITCHER_ABORTED)
        return 0;
    ++counter;
    ENSURE(counter == 1);
    switcher_wake(3, (nanosec_t)(2.0 * NOW_SECOND));
    return 0;
}


void *
r2()
{
    printf("Entered thread 2\n");
    int yd = switcher_yield(3, false);
    if (yd == SWITCHER_ABORTED)
        return 0;
    ++counter;
    ENSURE(counter == 2);
    return 0;
}


int
main()
{
    double elapsed;
    start = time(NULL);

    switcher_wake(2, (nanosec_t)(1.5 * NOW_SECOND));

    pthread_t t[2];
    pthread_create(&t[0], 0, r1, 0);
    pthread_create(&t[1], 0, r2, 0);

    pthread_join(t[0], 0);
    pthread_join(t[1], 0);

    finish  = time(NULL);
    elapsed = (double)(finish - start);
    printf("elapsed time = %.2lf", elapsed);

    ENSURE(counter == 2);
    ENSURE((elapsed - 4.00) <= 1e-6); // should be >= sleep + slack time
}

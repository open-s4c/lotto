#include <dispatch/dispatch.h>
#include <unistd.h>

#include <dice/interpose.h>

INTERPOSE(long, dispatch_semaphore_wait, dispatch_semaphore_t sem,
          dispatch_time_t timeout)
{
    static const char before[] = "native dispatch_semaphore_wait before\n";
    static const char after[]  = "native dispatch_semaphore_wait after\n";

    ssize_t ret = write(STDOUT_FILENO, before, sizeof(before) - 1);
    (void)ret;

    long wait_ret = REAL(dispatch_semaphore_wait, sem, timeout);

    ret = write(STDOUT_FILENO, after, sizeof(after) - 1);
    (void)ret;

    return wait_ret;
}

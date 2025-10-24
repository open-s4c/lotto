// clang-format off
// UNSUPPORTED: aarch64
// XFAIL: *
// RUN: exit 1
// clang-format on
#include <pthread.h>

int
main()
{
    pthread_mutex_t l;
    pthread_mutex_lock(&l);
    pthread_mutex_unlock(&l);
    return 0;
}

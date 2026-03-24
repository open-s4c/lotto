#include <signal.h>

#include <dice/interpose.h>

enum {
    MALLOC_SIGABORT_SIZE = 65521,
};

INTERPOSE(void *, malloc, size_t size)
{
    if (size == MALLOC_SIGABORT_SIZE) {
        raise(SIGABRT);
    }
    return REAL(malloc, size);
}

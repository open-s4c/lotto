#include <signal.h>
#include <unistd.h>

#include <dice/interpose.h>
#include <lotto/engine/prng.h>

enum {
    MALLOC_RANDOM_SIGABORT_SIZE = 65521,
};

INTERPOSE(void *, malloc, size_t size)
{
    if (size == MALLOC_RANDOM_SIGABORT_SIZE) {
        int trigger = (int)prng_range(0, 2);

        if (trigger) {
            write(STDOUT_FILENO, "MALLOC_RANDOM: 1\n",
                  sizeof("MALLOC_RANDOM: 1\n") - 1);
            raise(SIGABRT);
        }
        write(STDOUT_FILENO, "MALLOC_RANDOM: 0\n",
              sizeof("MALLOC_RANDOM: 0\n") - 1);
    }
    return REAL(malloc, size);
}

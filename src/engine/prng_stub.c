/*
 * Lightweight deterministic PRNG used by the pared-down engine build.
 */
#include <lotto/engine/prng.h>

static uint64_t seed = 1;

uint64_t
prng_seed(void)
{
    return seed;
}

uint64_t
prng_next(void)
{
    seed = lcg_next(seed);
    return seed;
}

uint64_t
prng_range(uint64_t min, uint64_t max)
{
    if (max <= min) {
        return min;
    }
    uint64_t range = max - min;
    return min + (prng_next() % range);
}

double
prng_real(void)
{
    return (double)prng_next() / (double)lcg_m;
}

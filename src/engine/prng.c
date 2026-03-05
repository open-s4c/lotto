#include <stdint.h>
#include <stdlib.h>
#include <lotto/engine/prng.h>
#include <lotto/states/prng.h>
#include <lotto/sys/assert.h>
#include <lotto/util/casts.h>

#define PRNG_MASK 0xFFFFFFFF

uint64_t
prng_seed()
{
    return prng()->seed;
}

// actually only returns  4 random bytes (32bit) due to & PRNG_MASK
uint64_t
prng_next()
{
    return (CAST_TYPE(uint64_t, rand_r(&prng()->seed)) & PRNG_MASK);
}

uint64_t
prng_range(uint64_t min, uint64_t max)
{
    ASSERT(max > min);
    uint64_t v     = prng_next();
    uint64_t range = max - min;
    uint64_t rn    = min + (v % range);
    return rn;
}

double
prng_real()
{
    double rn = 1.0 * (double)prng_next() / RAND_MAX;
    return rn;
}

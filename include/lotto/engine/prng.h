#ifndef LOTTO_PRNG_H
#define LOTTO_PRNG_H
/*******************************************************************************
 * @file prng.h
 * @brief Deterministic random generator for Lotto's engine.
 ******************************************************************************/
#include <stdint.h>

/**
 * Returns the random generator seed.
 */
uint64_t prng_seed(void);

/**
 * Returns the next random number.
 */
uint64_t prng_next(void);

/**
 * Returns the next random number adjusted to be in the range [min;max).
 */
uint64_t prng_range(uint64_t min, uint64_t max);

/**
 * Returns the next random number adjust between 0.0 and 1.0.
 */
double prng_real(void);

// LCG from glibc
// multiplier 1103515245
// increment 12345
// modulus 2^31
static const uint64_t lcg_a = 1103515245;
static const uint64_t lcg_c = 12345;
static const uint64_t lcg_m = 1LL << 31;

static inline uint64_t
lcg_next(uint64_t x)
{
    return ((lcg_a * x) + lcg_c) % lcg_m;
}

#endif

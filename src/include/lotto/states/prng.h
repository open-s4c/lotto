/*
 */
#ifndef LOTTO_STATE_PRNG_H
#define LOTTO_STATE_PRNG_H

#include <stdint.h>

#include <lotto/base/marshable.h>

typedef struct prng {
    marshable_t m;
    uint32_t seed;
} prng_t;

prng_t *prng();

#endif

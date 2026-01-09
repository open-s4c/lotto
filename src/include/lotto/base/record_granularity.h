/*
 */
#ifndef LOTTO_RECORD_GRANULARITY_H
#define LOTTO_RECORD_GRANULARITY_H

#include <stdbool.h>
#include <stdint.h>


enum record_granularity {
    RECORD_GRANULARITY_MINIMAL = 0x0,
    RECORD_GRANULARITY_SWITCH  = 0x1,
    RECORD_GRANULARITY_CHPT    = 0x2,
    RECORD_GRANULARITY_CAPTURE = 0x4,
    RECORD_GRANULARITY_ANY     = ~(0U),
};

typedef uint64_t record_granularities_t;

#define RECORD_GRANULARITIES_DEFAULT RECORD_GRANULARITY_MINIMAL

#define RECORD_GRANULARITIES_MAX_LEN 256

/**
 * Writes the given record granularity set string.
 *
 * @param granularities record granularity set
 * @param dst char array of at least RECORD_GRANULARITIES_MAX_LEN bytes
 */
void record_granularities_str(record_granularities_t granularities, char *dst);

/**
 * Returns bar-separated the record granularity set parsed from `src`.
 *
 * @param src string of the form `VAL|...`.
 * @return record granularity set
 */
record_granularities_t record_granularities_from(const char *src);

/**
 * Returns whether the record granularity set fully contains a given record
 * granularity.
 *
 * @param granularities record granularity set
 * @param granularity record granularity
 * @return true if element is in the granularity set, otherwise false
 */
bool record_granularities_have(record_granularities_t granularities,
                               enum record_granularity granularity);

#endif

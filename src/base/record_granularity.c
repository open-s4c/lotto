#include <ctype.h>
#include <string.h>

#include <lotto/base/record_granularity.h>
#include <lotto/base/string.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

static const char *_record_granularity_map[] = {
    [RECORD_GRANULARITY_MINIMAL] = "MINIMAL",
    [RECORD_GRANULARITY_SWITCH]  = "SWITCH",
    [RECORD_GRANULARITY_CHPT]    = "CHPT",
    [RECORD_GRANULARITY_CAPTURE] = "CAPTURE",
};

BITS_STR(record_granularity, record_granularities)
BITS_FROM(record_granularity, record_granularities, RECORD_GRANULARITY,
          RECORD_GRANULARITIES)
BITS_HAS(record_granularity, record_granularities)

bool
record_granularities_have(record_granularities_t granularities,
                          enum record_granularity granularity)
{
    return !granularity || granularities & granularity;
}

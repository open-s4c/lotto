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

void
record_granularities_str(record_granularities_t granularities, char *dst)
{
    if (granularities == RECORD_GRANULARITY_ANY) {
        strcpy(dst, "ANY");
        return;
    }
    if (granularities == RECORD_GRANULARITY_MINIMAL) {
        strcpy(dst, _record_granularity_map[RECORD_GRANULARITY_MINIMAL]);
        return;
    }

    dst[0] = '\0';
    for (uint64_t i = 1; i < sizeof(_record_granularity_map) / sizeof(char *);
         i <<= 1) {
        const char *val = _record_granularity_map[i];
        if (val == NULL)
            continue;
        if (i & granularities) {
            if (dst[0] != '\0')
                strcat(dst, "|");
            strcat(dst, val);
        }
    }
}

void
record_granularities_all_str(char *dst)
{
    strcpy(dst, "MINIMAL,SWITCH,CHPT,CAPTURE,ANY");
}

record_granularities_t
record_granularities_from(const char *src)
{
    record_granularities_t granularities = 0;
    char *token;
    const char *s                           = ":|,";
    char text[RECORD_GRANULARITIES_MAX_LEN] = {0};

    if (src == NULL)
        return RECORD_GRANULARITIES_DEFAULT;
    if (src[0] == '\0')
        return RECORD_GRANULARITY_INVALID;

    if (strlen(src) >= RECORD_GRANULARITIES_MAX_LEN)
        return RECORD_GRANULARITY_INVALID;
    strcat(text, src);
    strupr(text);
    if (strcmp(text, "ANY") == 0)
        return RECORD_GRANULARITY_ANY;

    token = strtok(text, s);

    for (; token != NULL; token = strtok(NULL, s)) {
        bool found = false;
        if (strcmp(token, "ANY") == 0) {
            granularities |= RECORD_GRANULARITY_ANY;
            continue;
        }
        for (uint64_t i = 0;
             token && i < sizeof(_record_granularity_map) / sizeof(char *);
             i = i ? i << 1 : 1) {
            if (strcmp(token, _record_granularity_map[i]) == 0) {
                granularities |= i;
                found = true;
                break;
            }
        }
        if (!found)
            return RECORD_GRANULARITY_INVALID;
    }

    return granularities;
}

BITS_HAS(record_granularity, record_granularities)

bool
record_granularities_have(record_granularities_t granularities,
                          enum record_granularity granularity)
{
    return !granularity || granularities & granularity;
}

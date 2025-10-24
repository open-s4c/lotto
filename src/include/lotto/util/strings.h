/*
 */

#ifndef LOTTO_STRINGS_H
#define LOTTO_STRINGS_H

#include <stdint.h>
#include <string.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/stdlib.h>
#include <lotto/util/casts.h>

static inline int
min(int a, int b, int c)
{
    if (a < b) {
        return (a < c) ? a : c;
    } else {
        return (b < c) ? b : c;
    }
}

static inline uint32_t
levenshtein(const char *s1, const char *s2)
{
    ASSERT(s1);
    ASSERT(s2);

    uint32_t len1 = CAST_TYPE(uint32_t, strlen(s1));
    uint32_t len2 = CAST_TYPE(uint32_t, strlen(s2));
    uint32_t **d  = (uint32_t **)sys_malloc((len1 + 1) * sizeof(uint32_t *));

    for (uint32_t i = 0; i <= len1; i++) {
        d[i] = (uint32_t *)sys_malloc((len2 + 1) * sizeof(uint32_t));
    }

    for (uint32_t i = 0; i <= len1; i++) {
        d[i][0] = i; // Deletion cost
    }
    for (uint32_t j = 0; j <= len2; j++) {
        d[0][j] = j; // Insertion cost
    }

    for (uint32_t i = 1; i <= len1; i++) {
        for (uint32_t j = 1; j <= len2; j++) {
            uint32_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j]       = min(d[i - 1][j] + 1,         // Deletion
                                d[i][j - 1] + 1,         // Insertion
                                d[i - 1][j - 1] + cost); // Substitution
        }
    }

    uint32_t distance = d[len1][len2];

    for (uint32_t i = 0; i <= len1; i++) {
        sys_free(d[i]);
    }
    sys_free(d);

    return distance;
}

#endif

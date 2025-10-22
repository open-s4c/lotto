#include <ctype.h>

#include <lotto/base/string.h>
#include <lotto/util/casts.h>

char *
strupr(char *s)
{
    for (int i = 0; s[i] != '\0'; i++) {
        s[i] = (char)toupper(s[i]);
    }
    return s;
}

/**
 * http://www.cse.yorku.ca/~oz/hash.html
 */
uint64_t
string_hash(const char *string)
{
    uint64_t hash = 5381;
    uint64_t c;

    while ((c = CAST_TYPE(uint64_t, (CAST_TYPE(uint8_t, *string))))) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        string++;
    }

    return hash;
}

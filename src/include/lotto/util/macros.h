#ifndef LOTTO_MACROS_H
#define LOTTO_MACROS_H

#include <dice/compiler.h>

#ifdef LOTTO_TEST
    #define NORETURN
#else
    #define NORETURN __attribute__((noreturn))
#endif

#define WEAK __attribute__((weak))

#if defined(LOTTO_TEST) || defined(LOTTO_DEBUG)
    #define STATIC
#else
    #define STATIC static
#endif

#define bool_str(B) ((B) ? "true" : "false")

#define LOTTO_NOP                                                              \
    do {                                                                       \
    } while (0)
#define LOTTO_CONSTRUCTOR_PRIO 101
#define LOTTO_CONSTRUCTOR      __attribute__((constructor(LOTTO_CONSTRUCTOR_PRIO)))
#define LOTTO_DESTRUCTOR       __attribute__((destructor))
#define LOTTO_WEAK             DICE_WEAK

#define NUM_TO_BIT(num) ((num) ? 1ULL << ((num)-1) : 0ULL)
#define IS_BIT(bit)     (!((bit) & ((bit)-1)))

#define BITS_FROM(bit, bits, BIT, BITS)                                        \
    bits##_t bits##_from(const char *src)                                      \
    {                                                                          \
        bits##_t bits = 0;                                                     \
        char *token;                                                           \
        const char *s             = ":|";                                      \
        char text[BITS##_MAX_LEN] = {0};                                       \
                                                                               \
        if (src == NULL)                                                       \
            return BITS##_DEFAULT;                                             \
        if (strcmp(src, "ANY") == 0)                                           \
            return BIT##_ANY;                                                  \
                                                                               \
        ASSERT(strlen(src) < BITS##_MAX_LEN);                                  \
        strcat(text, src);                                                     \
        strupr(text);                                                          \
                                                                               \
        token = strtok(text, s);                                               \
                                                                               \
        for (; token != NULL; token = strtok(NULL, s)) {                       \
            for (uint64_t i = 1;                                               \
                 token && i < sizeof(_##bit##_map) / sizeof(char *); i <<= 1)  \
                if (strcmp(token, _##bit##_map[i]) == 0) {                     \
                    bits |= i;                                                 \
                    break;                                                     \
                }                                                              \
        }                                                                      \
                                                                               \
        return bits;                                                           \
    }

#define BIT_FROM(bit, BIT)                                                     \
    bit##_t bit##_from(const char *src)                                        \
    {                                                                          \
        bit##_t i;                                                             \
        if (!src) {                                                            \
            return BIT##_NONE;                                                 \
        }                                                                      \
        for (i = BIT##_NONE; i < BIT##_END_; i++) {                            \
            if (strcmp(src, _##bit##_map[i]) == 0) {                           \
                return i;                                                      \
            }                                                                  \
        }                                                                      \
        ASSERT(0 && "could not parse the " #bit);                              \
        return BIT##_NONE;                                                     \
    }

#define BITS_STR(bit, bits)                                                    \
    void bits##_str(bits##_t bits, char *dst)                                  \
    {                                                                          \
        dst[0] = '\0';                                                         \
        for (uint64_t i = 1; i < sizeof(_##bit##_map) / sizeof(char *);        \
             i <<= 1) {                                                        \
            const char *val = _##bit##_map[i];                                 \
            if (val == NULL)                                                   \
                continue;                                                      \
            if (i & bits) {                                                    \
                if (dst[0] != '\0')                                            \
                    strcat(dst, "|");                                          \
                strcat(dst, val);                                              \
            }                                                                  \
        }                                                                      \
    }

#define BIT_STR(bit, BIT)                                                      \
    const char *bit##_str(enum bit bit)                                        \
    {                                                                          \
        ASSERT(bit < BIT##_END_);                                              \
        ASSERT(_##bit##_map[bit]);                                             \
        return _##bit##_map[bit];                                              \
    }

#define BITS_HAS(bit, bits)                                                    \
    bool bits##_has(bits##_t bits, enum bit bit)                               \
    {                                                                          \
        return !bit || (bits & bit);                                           \
    }

#define BITS_HAS_NONEMPTY(bit, bits)                                           \
    bool bits##_has(bits##_t bits, enum bit bit)                               \
    {                                                                          \
        return (bits & bit);                                                   \
    }

#define BIT_ALL_STR(bit, BIT)                                                  \
    void bit##_all_str(char *output)                                           \
    {                                                                          \
        bool first = true;                                                     \
        bit##_t i;                                                             \
        for (i = BIT##_NONE; i < BIT##_END_; i++) {                            \
            if (!_##bit##_map[i])                                              \
                continue;                                                      \
            if (!first) {                                                      \
                strcpy(output, "|");                                           \
                output++;                                                      \
            }                                                                  \
            strcpy(output, _##bit##_map[i]);                                   \
            output += strlen(_##bit##_map[i]);                                 \
            first = false;                                                     \
        }                                                                      \
    }

#ifndef MIN
    #define MIN(A, B) ((A) > (B) ? (B) : (A))
#endif

#ifndef MAX
    #define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

#define CONCAT(X, Y)       CONCAT_AGAIN(X, Y)
#define CONCAT_AGAIN(X, Y) X##Y

#define XSTR(s) STR(s)
#define STR(s)  #s

#endif

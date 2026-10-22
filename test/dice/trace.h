#ifndef LOTTO_TEST_DICE_TRACE_H
#define LOTTO_TEST_DICE_TRACE_H
#include <assert.h>
#include <string.h>

#include <lotto/dice.h>

extern char initialization[16];
extern char delivery[16];
extern unsigned replies;

static inline void
record(char *trace, char value)
{
    size_t length = strlen(trace);
    assert(length + 1 < 16);
    trace[length]     = value;
    trace[length + 1] = 0;
}
#endif

/*
 */
#ifndef LOTTO_CLK_H
#define LOTTO_CLK_H

#include <stdint.h>
#include <stdlib.h>

typedef uint64_t clk_t;

static inline clk_t
clk_parse(const char *src)
{
    return src == NULL ? 0 : (clk_t)atoi(src);
}

#endif

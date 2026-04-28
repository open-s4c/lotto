#ifndef LOTTO_UAFCHECK_STATE_H
#define LOTTO_UAFCHECK_STATE_H

#include <stddef.h>

#include <lotto/base/marshable.h>

#define UAFCHECK_DEFAULT_MAX_PAGES 16
#define UAFCHECK_DEFAULT_PROB      0.5

typedef struct uafcheck_config {
    marshable_t m;
    size_t max_pages;
    double prob;
} uafcheck_config_t;

uafcheck_config_t *uafcheck_config(void);

#endif

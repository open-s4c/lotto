/**
 * @file state.h
 * @brief Sampling module state declarations.
 */
#ifndef LOTTO_STATE_SAMPLING_H
#define LOTTO_STATE_SAMPLING_H

#include <limits.h>
#include <stdbool.h>

#include <lotto/base/bag.h>
#include <lotto/base/marshable.h>

#define SAMPLING_NAME_MAX_LEN 256

struct sampling_config_entry {
    bagitem_t bi;
    double p;
    char name[SAMPLING_NAME_MAX_LEN + 1];
};

struct sampling_config {
    marshable_t m;
    char filename[PATH_MAX];
};

struct sampling_config *sampling_config();
bag_t *sampling_entries(void);
void sampling_config_print(void);

#endif

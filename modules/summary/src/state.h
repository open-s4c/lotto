/**
 * @file state.h
 * @brief Summary module state declarations.
 */
#ifndef LOTTO_STATE_SUMMARY_H
#define LOTTO_STATE_SUMMARY_H

#include <limits.h>

#include <lotto/base/marshable.h>

typedef struct summary_config {
    marshable_t m;
    char csv[PATH_MAX];
} summary_config_t;

summary_config_t *summary_config();

#endif

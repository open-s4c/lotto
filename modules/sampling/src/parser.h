/**
 * @file parser.h
 * @brief Sampling configuration parser.
 */
#ifndef LOTTO_MODULES_SAMPLING_PARSER_H
#define LOTTO_MODULES_SAMPLING_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include <lotto/base/bag.h>

typedef void (*sampling_parse_error_f)(size_t line, const char *msg,
                                       const char *detail, void *arg);

bool sampling_parse_config(char *text, bag_t *entries,
                           sampling_parse_error_f report, void *arg);

#endif

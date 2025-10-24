/*
 */
#ifndef LOTTO_CLI_UTILS_H
#define LOTTO_CLI_UTILS_H

#include <stdbool.h>
#include <stdint.h>

void remove_all(const char *pattern);
bool adjust(const char *fn);
uint64_t get_lotto_hash(const char *arg0);

#endif

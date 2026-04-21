/**
 * @file utils.h
 * @brief Driver declarations for utils.
 */
#ifndef LOTTO_DRIVER_UTILS_H
#define LOTTO_DRIVER_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/driver/args.h>
#include <lotto/driver/flagmgr.h>

void remove_all(const char *pattern);
void round_print(const flags_t *flags, uint64_t round);
bool adjust(const char *fn);
uint64_t get_lotto_hash(const char *arg0);
flags_t *run_default_flags();
int run_once(args_t *args, flags_t *flags);

#endif

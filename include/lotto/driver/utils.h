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
typedef int(run_post_run_hook_f)(const args_t *args, const flags_t *flags,
                                 int ret);
void run_set_post_run_hook(run_post_run_hook_f *hook);
flags_t *run_default_flags();
int cp(const char *from, const char *to);
int run_once(args_t *args, flags_t *flags);

#endif

/**
 * @file exec.h
 * @brief Driver declarations for exec.
 */
#ifndef LOTTO_DRIVER_EXEC_H
#define LOTTO_DRIVER_EXEC_H

#include <lotto/base/flags.h>
#include <lotto/driver/args.h>

int execute(const args_t *args, const flags_t *flags, bool config);

#endif

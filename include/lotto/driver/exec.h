/**
 * @file exec.h
 * @brief Driver declarations for exec.
 */
#ifndef LOTTO_DRIVER_EXEC_H
#define LOTTO_DRIVER_EXEC_H

#include <lotto/base/flags.h>
#include <lotto/driver/args.h>

/** Hook used to rewrite a command line before `execute()` spawns the child. */
typedef void(exec_command_prefix_f)(args_t *args, const flags_t *flags);

/** Set or clear the process launch prefix hook used by `execute()`. */
void execute_set_command_prefix(exec_command_prefix_f *prefix);
/** Spawn and supervise a command according to current driver settings. */
int execute(const args_t *args, const flags_t *flags, bool config);

#endif

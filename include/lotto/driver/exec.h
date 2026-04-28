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
/** Hook used to rewrite recorded replay args before `execute()` runs. */
typedef void(exec_replay_args_resolver_f)(args_t *args, const flags_t *flags);
/** Hook used to decide whether child stdin should come from `/dev/null`. */
typedef bool(exec_stdin_devnull_f)(const flags_t *flags);

/** Set or clear the process launch prefix hook used by `execute()`. */
void execute_set_command_prefix(exec_command_prefix_f *prefix);
/** Set or clear replay arg resolver hook used by replay-like commands. */
void execute_set_replay_args_resolver(exec_replay_args_resolver_f *resolver);
/** Set or clear stdin-detach hook used by `execute()`. */
void execute_set_stdin_devnull(exec_stdin_devnull_f *hook);
/** Resolve replay args through the currently registered replay hook. */
void execute_resolve_replay_args(args_t *args, const flags_t *flags);
/** Spawn and supervise a command according to current driver settings. */
int execute(const args_t *args, const flags_t *flags, bool config);

#endif

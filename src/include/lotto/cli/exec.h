/*
 */
#ifndef LOTTO_CLI_EXEC_H
#define LOTTO_CLI_EXEC_H

#include <lotto/base/flags.h>
#include <lotto/cli/args.h>

int execute(const args_t *args, const flags_t *flags, bool config);

#endif

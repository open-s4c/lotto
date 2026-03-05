#ifndef LOTTO_CLI_STRESS_H
#define LOTTO_CLI_STRESS_H

#include <lotto/cli/args.h>
#include <lotto/cli/flagmgr.h>

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_ROUNDS;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_BEFORE_RUN;
DECLARE_FLAG_AFTER_RUN;
DECLARE_FLAG_LOGGER_FILE;

int stress(args_t *args, flags_t *flags);

static inline flags_t *
_stress_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, FLAG_INPUT, sval(""));
    return flags;
}

#endif

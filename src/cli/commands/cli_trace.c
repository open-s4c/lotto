/*
 */
/*******************************************************************************
 * trace
 ******************************************************************************/
#include <limits.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/cli/args.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_REPLAY_GOAL;
DECLARE_FLAG_CREP;

/**
 * generate initial trace
 */
int
trace(args_t *args, flags_t *flags)
{
    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD), flags_is_on(flags, FLAG_CREP),
            false, flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, FLAG_INPUT)},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {NULL}};
    envvar_set(vars, true);

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] creating initial trace for: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    struct value val = uval(now());
    flags_set_by_opt(flags, flag_seed(), val);
    cli_trace_init(flags_get_sval(flags, FLAG_OUTPUT), args,
                   flags_get_sval(flags, FLAG_INPUT),
                   flags_get(flags, FLAG_REPLAY_GOAL), true, flags);
    char tmp_name[PATH_MAX];
    sys_sprintf(tmp_name, "%s.tmp",
                flags_get_sval(flags, FLAG_INPUT) &&
                        flags_get_sval(flags, FLAG_INPUT)[0] ?
                    flags_get_sval(flags, FLAG_INPUT) :
                    flags_get_sval(flags, FLAG_OUTPUT));
    rename(tmp_name, flags_get_sval(flags, FLAG_OUTPUT));

    return 0;
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, FLAG_INPUT, sval(""));
    return flags;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_INPUT,
                    FLAG_OUTPUT,
                    FLAG_VERBOSE,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_REPLAY_GOAL,
                    FLAG_CREP,
                    0};
    subcmd_register(trace, "trace", "[--] <command line>",
                    "Create an initial trace for running Lotto without CLI",
                    true, sel, _default_flags, SUBCMD_GROUP_RUN);
}

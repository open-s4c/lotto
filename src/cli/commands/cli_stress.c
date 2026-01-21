/*
 */
/*******************************************************************************
 * stress
 ******************************************************************************/
#include <dirent.h>
#include <limits.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/cli/args.h>
#include <lotto/cli/cli_stress.h>
#include <lotto/cli/exec.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/memmgr.h>
#include <lotto/cli/flags/prng.h>
#include <lotto/cli/preload.h>
#include <lotto/cli/subcmd.h>
#include <lotto/cli/trace_utils.h>
#include <lotto/cli/utils.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>

void round_print(flags_t *flags, uint64_t round);

/**
 * stress test
 */
int
stress(args_t *args, flags_t *flags)
{
    setenv("LOTTO_LOGGER_FILE", flags_get_sval(flags, FLAG_LOGGER_FILE), true);

    preload(flags_get_sval(flags, FLAG_TEMPORARY_DIRECTORY),
            flags_is_on(flags, FLAG_VERBOSE),
            !flags_is_on(flags, FLAG_NO_PRELOAD),
            flags_is_on(flags, FLAG_FLOTTO),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, FLAG_OUTPUT)},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, FLAG_LOGGER_BLOCK)},
        {NULL}};
    envvar_set(vars, true);

    if (flags_is_on(flags, FLAG_VERBOSE)) {
        sys_fprintf(stdout, "[lotto] starting: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    /* Continue the stress from a previous trace.
     *
     * Note that 'stress -i trace' does not guarantee the same output
     * trace on a second invocation. */
    const char *input = flags_get_sval(flags, FLAG_INPUT);
    if (input && input[0] != '\0') {
        adjust(input);
    }

    struct flag_val seed = flags_get(flags, flag_seed());

    int err         = 0;
    uint64_t rounds = flags_get_uval(flags, FLAG_ROUNDS);
    for (uint64_t i = 0; i < rounds; i++) {
        round_print(flags, i);

        if ((err = execute(args, flags, false)))
            return err;

        if (rounds > 1) {
            adjust(flags_get_sval(flags, FLAG_OUTPUT));
        }

        /* if no seed is given as argument, we use time as seed for the next
         * round, otherwise, we increment the initial seed before every round.
         */
        if (seed.is_default)
            seed._val = uval(now());
        else
            seed._val = uval(as_uval(seed._val) + 1);


        flags_set_by_opt(flags, flag_seed(), seed._val);
    }
    return 0;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_OUTPUT,
                    FLAG_INPUT,
                    FLAG_VERBOSE,
                    FLAG_ROUNDS,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,
                    FLAG_AFTER_RUN,
                    FLAG_LOGGER_FILE,
                    FLAG_FLOTTO,
                    0};
    subcmd_register(stress, "stress", "[--] <command line>",
                    "Stress test a program until a desired execution is found",
                    true, sel, _stress_default_flags, SUBCMD_GROUP_RUN);
}

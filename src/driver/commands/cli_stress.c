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
#include <lotto/cli/cli_stress.h>
#include <lotto/driver/preload.h>
#include <lotto/driver/args.h>
#include <lotto/driver/exec.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>

void round_print(flags_t *flags, uint64_t round);

flags_t *
stress_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_input(), sval(""));
    return flags;
}

/**
 * stress test
 */
int
stress(args_t *args, flags_t *flags)
{
    setenv("LOTTO_LOGGER_FILE", flags_get_sval(flags, flag_logger_file()),
           true);

    preload(flags_get_sval(flags, flag_temporary_directory()),
            flags_is_on(flags, flag_verbose()),
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, flag_output())},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, flag_logger_block())},
        {NULL}};
    envvar_set(vars, true);

    if (flags_is_on(flags, flag_verbose())) {
        sys_fprintf(stdout, "[lotto] starting: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    /* Continue the stress from a previous trace.
     *
     * Note that 'stress -i trace' does not guarantee the same output
     * trace on a second invocation. */
    const char *input = flags_get_sval(flags, flag_input());
    if (input && input[0] != '\0') {
        adjust(input);
    }

    struct flag_val seed = flags_get(flags, flag_seed());

    int err         = 0;
    uint64_t rounds = flags_get_uval(flags, flag_rounds());
    for (uint64_t i = 0; i < rounds; i++) {
        round_print(flags, i);

        if ((err = execute(args, flags, false)))
            return err;

        if (rounds > 1) {
            adjust(flags_get_sval(flags, flag_output()));
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

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_output(),
                    flag_input(),
                    flag_verbose(),
                    flag_rounds(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_logger_block(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(stress, "stress", "[--] <command line>",
                    "Stress test a program until a desired execution is found",
                    true, sel, stress_default_flags, SUBCMD_GROUP_RUN);
})

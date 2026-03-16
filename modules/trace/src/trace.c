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
#include <lotto/cli/preload.h>
#include <lotto/driver/args.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace_prepare.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>

int
trace(args_t *args, flags_t *flags)
{
    preload(flags_get_sval(flags, flag_temporary_directory()),
            flags_is_on(flags, flag_verbose()),
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = flags_get_sval(flags, flag_input())},
        {"LOTTO_RECORD", .sval = flags_get_sval(flags, flag_output())},
        {"LOTTO_LOGGER_BLOCK",
         .sval = flags_get_sval(flags, flag_logger_block())},
        {NULL}};
    envvar_set(vars, true);

    if (flags_is_on(flags, flag_verbose())) {
        sys_fprintf(stdout, "[lotto] creating initial trace for: ");
        args_print(args);
        sys_fprintf(stdout, "\n");
    }

    struct value val = uval(now());
    flags_set_by_opt(flags, flag_seed(), val);
    cli_trace_init(flags_get_sval(flags, flag_output()), args,
                   flags_get_sval(flags, flag_input()),
                   flags_get(flags, flag_replay_goal()), true, flags);
    char tmp_name[PATH_MAX];
    sys_sprintf(tmp_name, "%s.tmp",
                flags_get_sval(flags, flag_input()) &&
                        flags_get_sval(flags, flag_input())[0] ?
                    flags_get_sval(flags, flag_input()) :
                    flags_get_sval(flags, flag_output()));
    rename(tmp_name, flags_get_sval(flags, flag_output()));

    return 0;
}

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_input(), sval(""));
    return flags;
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__REGISTER_COMMANDS, {
    flag_t sel[] = {flag_input(),       flag_output(),
                    flag_verbose(),     flag_temporary_directory(),
                    flag_no_preload(),  flag_logger_block(),
                    flag_replay_goal(), 0};
    subcmd_register(trace, "trace", "[--] <command line>",
                    "Create an initial trace for running Lotto without CLI",
                    true, sel, _default_flags, SUBCMD_GROUP_RUN);
})

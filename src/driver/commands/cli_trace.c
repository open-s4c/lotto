/*******************************************************************************
 * start / config
 ******************************************************************************/
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/envvar.h>
#include <lotto/driver/args.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/memmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/preload.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/trace_prepare.h>
#include <lotto/sys/now.h>
#include <lotto/sys/stdio.h>

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_input(), sval(""));
    return flags;
}

static int
_trace_prepare(args_t *args, flags_t *flags, bool requires_program)
{
    uint64_t verbose = flag_verbose_count(flags);
    const char *input = flags_get_sval(flags, flag_input());
    const char *output = flags_get_sval(flags, flag_output());

    preload(flags_get_sval(flags, flag_temporary_directory()), verbose,
            !flags_is_on(flags, flag_no_preload()),
            flags_get_sval(flags, flag_memmgr_runtime()),
            flags_get_sval(flags, flag_memmgr_user()));

    envvar_t vars[] = {
        {"LOTTO_REPLAY", .sval = input},
        {"LOTTO_RECORD", .sval = output},
        {NULL}};
    envvar_set(vars, true);

    if (verbose > 0) {
        if (requires_program) {
            sys_fprintf(stdout, "[lotto] creating start trace for: ");
            args_print(args);
            sys_fprintf(stdout, "\n");
        } else {
            sys_fprintf(stdout, "[lotto] appending config to: %s\n", input);
        }
    }

    struct value val = uval(now());
    flags_set_by_opt(flags, flag_seed(), val);
    cli_trace_init(output, args, input, flags_get(flags, flag_replay_goal()),
                   true, flags);
    char tmp_name[PATH_MAX];
    sys_sprintf(tmp_name, "%s.tmp", input && input[0] ? input : output);
    if (rename(tmp_name, output) != 0) {
        sys_fprintf(stderr, "error: could not rename %s to %s: %s\n", tmp_name,
                    output, strerror(errno));
        return 1;
    }

    return 0;
}

int
start(args_t *args, flags_t *flags)
{
    const char *input = flags_get_sval(flags, flag_input());
    if (input && input[0]) {
        sys_fprintf(stderr, "error: start does not accept an input trace\n");
        return 1;
    }
    return _trace_prepare(args, flags, true);
}

int
config(args_t *args, flags_t *flags)
{
    (void)args;
    const char *input = flags_get_sval(flags, flag_input());
    if (!(input && input[0])) {
        sys_fprintf(stderr, "error: config requires an input trace\n");
        return 1;
    }
    return _trace_prepare(args, flags, false);
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_input(),
                    flag_output(),
                    flag_verbose(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_replay_goal(),
                    0};
    subcmd_register(start, "start", "[--] <command line>",
                    "Create a trace containing START and CONFIG", true, sel,
                    _default_flags, SUBCMD_GROUP_RUN);
    subcmd_register(config, "config", "",
                    "Append a CONFIG record to an existing trace", true, sel,
                    _default_flags, SUBCMD_GROUP_TRACE);
})

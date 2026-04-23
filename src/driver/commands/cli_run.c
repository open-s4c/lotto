/*******************************************************************************
 * run
 ******************************************************************************/
#include <lotto/driver/subcmd.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/pubsub.h>

int
run(args_t *args, flags_t *flags)
{
    flags_set_by_opt(flags, flag_output(), sval(""));
    return run_once(args, flags);
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_verbose(),    flag_temporary_directory(),
                    flag_no_preload(), flag_before_run(),
                    flag_after_run(),  flag_logger_file(),
                    flag_input(),      0};
    subcmd_register(run, "run", "[--] <command line>",
                    "Run a program once without recording a trace",
                    true, sel, run_default_flags, SUBCMD_GROUP_RUN);
})

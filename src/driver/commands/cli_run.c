/*******************************************************************************
 * record
 ******************************************************************************/
#include <dirent.h>
#include <string.h>

#include <lotto/cli/cli_stress.h>
#include <lotto/driver/subcmd.h>
#include <lotto/engine/pubsub.h>

int
run(args_t *args, flags_t *flags)
{
    flags_set_by_opt(flags, flag_rounds(), uval(1));
    flags_set_by_opt(flags, flag_output(), sval(""));
    return stress(args, flags);
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_verbose(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_logger_block(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    flag_input(),
                    0};
    subcmd_register(run, "run", "[--] <command line>", "Run a program once",
                    true, sel, stress_default_flags, SUBCMD_GROUP_RUN);
})

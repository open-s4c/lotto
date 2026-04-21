/*******************************************************************************
 * record
 ******************************************************************************/
#include <lotto/driver/subcmd.h>
#include <lotto/driver/utils.h>
#include <lotto/engine/pubsub.h>

int
record(args_t *args, flags_t *flags)
{
    return run_once(args, flags);
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_output(),
                    flag_input(),
                    flag_verbose(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(record, "record", "[--] <command line>",
                    "Record a single execution of a program", true, sel,
                    run_default_flags, SUBCMD_GROUP_RUN);
})

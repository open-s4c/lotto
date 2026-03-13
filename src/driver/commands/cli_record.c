/*******************************************************************************
 * record
 ******************************************************************************/
#include <dirent.h>

#include <lotto/cli/cli_stress.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>
#include <lotto/engine/pubsub.h>

int
record(args_t *args, flags_t *flags)
{
    flags_set_by_opt(flags, flag_rounds(), uval(1));
    return stress(args, flags);
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_LOTTO_INIT, {
    flag_t sel[] = {flag_output(),              flag_input(),
                    flag_verbose(),             flag_temporary_directory(),
                    flag_no_preload(),          flag_logger_block(),
                    flag_before_run(),          flag_after_run(),
                    flag_logger_file(),         0};
    subcmd_register(record, "record", "[--] <command line>",
                    "Record a single execution of a program", true, sel,
                    stress_default_flags, SUBCMD_GROUP_RUN);
})

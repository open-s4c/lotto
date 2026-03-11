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
    flags_set_by_opt(flags, FLAG_ROUNDS, uval(1));
    return stress(args, flags);
}

LOTTO_SUBSCRIBE_CONTROL(EVENT_DRIVER__INIT, {
    flag_t sel[] = {FLAG_OUTPUT,      FLAG_INPUT,
                    FLAG_VERBOSE,     FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,  FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,  FLAG_AFTER_RUN,
                    FLAG_LOGGER_FILE, 0};
    subcmd_register(record, "record", "[--] <command line>",
                    "Record a single execution of a program", true, sel,
                    _stress_default_flags, SUBCMD_GROUP_RUN);
})

/*
 */
/*******************************************************************************
 * record
 ******************************************************************************/
#include <dirent.h>
#include <string.h>

#include <lotto/cli/cli_stress.h>
#include <lotto/cli/subcmd.h>

int
record(args_t *args, flags_t *flags)
{
    flags_set_by_opt(flags, FLAG_ROUNDS, uval(1));
    return stress(args, flags);
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_OUTPUT,      FLAG_INPUT,
                    FLAG_VERBOSE,     FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,  FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,  FLAG_AFTER_RUN,
                    FLAG_LOGGER_FILE, 0};
    subcmd_register(record, "record", "[--] <command line>",
                    "Record a single execution of a program", true, sel,
                    _stress_default_flags, SUBCMD_GROUP_RUN);
}

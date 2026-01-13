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
run(args_t *args, flags_t *flags)
{
    flags_set_by_opt(flags, FLAG_ROUNDS, uval(1));
    flags_set_by_opt(flags, FLAG_OUTPUT, sval(""));
    return stress(args, flags);
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_VERBOSE,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_BEFORE_RUN,
                    FLAG_AFTER_RUN,
                    FLAG_LOGGER_FILE,
                    FLAG_CREP,
                    FLAG_FLOTTO,
                    FLAG_INPUT,
                    0};
    subcmd_register(run, "run", "[--] <command line>", "Run a program once",
                    true, sel, _stress_default_flags, SUBCMD_GROUP_RUN);
}

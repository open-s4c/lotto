#include <lotto/cli/explore.h>
#include <lotto/cli/flagmgr.h>
#include <lotto/cli/flags/sequencer.h>
#include <lotto/cli/subcmd.h>

DECLARE_FLAG_OUTPUT;
DECLARE_FLAG_INPUT;
DECLARE_FLAG_VERBOSE;
DECLARE_FLAG_TEMPORARY_DIRECTORY;
DECLARE_FLAG_NO_PRELOAD;
DECLARE_FLAG_LOGGER_BLOCK;
DECLARE_FLAG_LOGGER_FILE;

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_strategy(), sval("random"));
    return flags;
}

static void LOTTO_CONSTRUCTOR
init()
{
    flag_t sel[] = {FLAG_OUTPUT,
                    FLAG_INPUT,
                    FLAG_VERBOSE,
                    FLAG_TEMPORARY_DIRECTORY,
                    FLAG_NO_PRELOAD,
                    FLAG_LOGGER_BLOCK,
                    FLAG_EXPLORE_EXPECT_FAILURE,
                    FLAG_EXPLORE_MIN,
                    FLAG_LOGGER_FILE,
                    0};
    subcmd_register(explore, "explore", "", "Exhaustively explore a trace",
                    true, sel, _default_flags, SUBCMD_GROUP_TRACE);
}

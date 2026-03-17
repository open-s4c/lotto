#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/sequencer.h>
#include <lotto/driver/subcmd.h>
#include <lotto/modules/explore/explore.h>

static flags_t *
_default_flags()
{
    flags_t *flags = flagmgr_flags_alloc();
    flags_cpy(flags, flags_default());
    flags_set_default(flags, flag_input(), sval("replay.trace"));
    flags_set_default(flags, flag_strategy(), sval("random"));
    return flags;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_output(),
                    flag_input(),
                    flag_verbose(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_logger_block(),
                    FLAG_EXPLORE_EXPECT_FAILURE,
                    FLAG_EXPLORE_MIN,
                    flag_logger_file(),
                    0};
    subcmd_register(explore, "explore", "", "Exhaustively explore a trace",
                    true, sel, _default_flags, SUBCMD_GROUP_TRACE);
})

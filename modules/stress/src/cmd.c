/*******************************************************************************
 * stress
 ******************************************************************************/
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/flags/prng.h>
#include <lotto/driver/subcmd.h>
#include <lotto/driver/utils.h>
#include <lotto/sys/now.h>
#include <lotto/engine/pubsub.h>

int
stress(args_t *args, flags_t *flags)
{
    struct flag_val seed = flags_get(flags, flag_seed());
    uint64_t rounds      = flags_get_uval(flags, flag_rounds());

    for (uint64_t i = 0; i < rounds; i++) {
        round_print(flags, i);

        int err = run_once(args, flags);
        if (err) {
            return err;
        }

        if (rounds > 1) {
            adjust(flags_get_sval(flags, flag_output()));
        }

        if (seed.is_default) {
            seed._val = uval(now());
        } else {
            seed._val = uval(as_uval(seed._val) + 1);
        }

        flags_set_by_opt(flags, flag_seed(), seed._val);
    }

    return 0;
}

ON_DRIVER_REGISTER_COMMANDS({
    flag_t sel[] = {flag_output(),
                    flag_input(),
                    flag_verbose(),
                    flag_rounds(),
                    flag_temporary_directory(),
                    flag_no_preload(),
                    flag_before_run(),
                    flag_after_run(),
                    flag_logger_file(),
                    0};
    subcmd_register(stress, "stress", "[--] <command line>",
                    "Stress test a program until a desired execution is found",
                    true, sel, run_default_flags, SUBCMD_GROUP_RUN);
})

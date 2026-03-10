#ifndef LOTTO_MODULES_EXPLORE_H
#define LOTTO_MODULES_EXPLORE_H

#include <lotto/driver/args.h>
#include <lotto/driver/flagmgr.h>

DECLARE_COMMAND_FLAG(EXPLORE_EXPECT_FAILURE, "", "explore-failure", "",
                     "expect all runs to fail", flag_off())
DECLARE_COMMAND_FLAG(EXPLORE_MIN, "", "explore-min", "UINT",
                     "minimum clock for explore", flag_uval(0))

int explore(args_t *args, flags_t *flags);

#endif

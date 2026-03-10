#ifndef LOTTO_DRIVER_TRACE_PREPARE_H
#define LOTTO_DRIVER_TRACE_PREPARE_H

#include <lotto/base/flags.h>
#include <lotto/driver/args.h>

#define MAX_GOAL (~(clk_t)0)

void cli_trace_init(const char *record, const args_t *args, const char *replay,
                    struct flag_val fgoal, bool config, const flags_t *flags);

#endif

/**
 * @file cli_stress.h
 * @brief Internal CLI declarations for cli stress.
 */
#ifndef LOTTO_CLI_STRESS_H
#define LOTTO_CLI_STRESS_H

#include <lotto/driver/args.h>
#include <lotto/driver/flagmgr.h>

int stress(args_t *args, flags_t *flags);
flags_t *stress_default_flags();
flag_t flag_input();
flag_t flag_output();
flag_t flag_verbose();
flag_t flag_rounds();
flag_t flag_temporary_directory();
flag_t flag_no_preload();
flag_t flag_logger_block();
flag_t flag_before_run();
flag_t flag_after_run();
flag_t flag_logger_file();

#endif

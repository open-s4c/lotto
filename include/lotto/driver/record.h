/**
 * @file record.h
 * @brief Driver declarations for record.
 */
#ifndef LOTTO_DRIVER_RECORD_H
#define LOTTO_DRIVER_RECORD_H

#include <lotto/base/record.h>
#include <lotto/driver/args.h>

record_t *record_config(clk_t goal);
record_t *record_start(const args_t *args);
void record_print(const record_t *r, int i);
args_t *record_args(const record_t *r);

#endif

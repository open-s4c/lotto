/**
 * @file runtime.h
 * @brief Runtime declarations for runtime.
 */
#ifndef LOTTO_RUNTIME_H
#define LOTTO_RUNTIME_H

#include <stdbool.h>

#include <lotto/base/reason.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/util/macros.h>

bool lotto_runtime_initialized(void);
bool lotto_runtime_bootstrapping(void);
void lotto_runtime_bootstrap_begin(void);
void lotto_runtime_bootstrap_end(void);
void lotto_exit(capture_point *cp, reason_t reason) NORETURN;

#endif

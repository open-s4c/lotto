/**
 * @file runtime.h
 * @brief Runtime declarations for runtime.
 */
#ifndef LOTTO_RUNTIME_H
#define LOTTO_RUNTIME_H

#include <lotto/base/reason.h>
#include <lotto/runtime/capture_point.h>

void lotto_exit(capture_point *cp, reason_t reason) NORETURN;

#endif

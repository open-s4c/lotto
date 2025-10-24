/*
 */
#ifndef LOTTO_RUNTIME_H
#define LOTTO_RUNTIME_H

#include <lotto/base/context.h>
#include <lotto/base/reason.h>

void lotto_exit(context_t *ctx, reason_t reason) NORETURN;

#endif

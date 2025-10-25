#ifndef LOTTO_RUNTIME_RUNTIME_H
#define LOTTO_RUNTIME_RUNTIME_H

#include <lotto/base/context.h>
#include <lotto/base/reason.h>

void lotto_exit(context_t *ctx, reason_t reason) __attribute__((noreturn));
void lotto_set_interceptor_initialized(void);

#endif /* LOTTO_RUNTIME_RUNTIME_H */

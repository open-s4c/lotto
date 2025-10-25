#include <stdlib.h>

#include <dice/module.h>

#include <lotto/base/context.h>
#include <lotto/base/reason.h>
#include <lotto/engine/engine.h>
#include "interceptor.h"
#include <lotto/runtime/runtime.h>

int
lotto_runtime_init(void)
{
    return 0;
}

void
lotto_exit(context_t *ctx, reason_t reason)
{
    int status = engine_fini(ctx, reason);
    exit(status);
}

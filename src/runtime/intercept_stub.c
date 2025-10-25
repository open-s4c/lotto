#include <dice/log.h>
#include <lotto/runtime/intercept.h>

mediator_t *
intercept_before_call(context_t *ctx)
{
	log_info("I'm HERE!!!");
    (void)ctx;
    return NULL;
}

void
intercept_after_call(const char *func)
{
    (void)func;
}

void
intercept_capture(context_t *ctx)
{
    (void)ctx;
}

bool
lotto_intercept_initialized(void)
{
    return false;
}


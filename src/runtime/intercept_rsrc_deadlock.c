#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>

void
_lotto_rsrc_acquiring(void *addr)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = __FUNCTION__;
        c->cat       = CAT_RSRC_ACQUIRING;
        c->args[0]   = arg_ptr(addr);
        intercept_capture(c);
    }
}

void
_lotto_rsrc_released(void *addr)
{
    if (intercept_capture != NULL) {
        context_t *c = ctx();
        c->func      = __FUNCTION__;
        c->cat       = CAT_RSRC_RELEASED;
        c->args[0]   = arg_ptr(addr);
        intercept_capture(c);
    }
}

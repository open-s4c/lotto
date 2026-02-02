#include <lotto/base/category.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>

void
_lotto_rsrc_acquiring(void *addr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_RSRC_ACQUIRING,
                          .args = {arg_ptr(addr)}));
}

void
_lotto_rsrc_released(void *addr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_RSRC_RELEASED,
                          .args = {arg_ptr(addr)}));
}

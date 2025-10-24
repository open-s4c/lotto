/*
 */
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/unsafe/rogue.h>

void
_lotto_region_rogue_enter()
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_TASK_BLOCK));
}

void
_lotto_region_rogue_leave()
{
    intercept_after_call(__FUNCTION__);
}

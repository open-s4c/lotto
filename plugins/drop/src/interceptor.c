/*
 */
#include <category.h>

#include <lotto/base/context.h>
#include <lotto/drop.h>
#include <lotto/runtime/intercept.h>
#include <lotto/util/once.h>

static category_t CAT_DROP;

void
_lotto_drop()
{
    once(CAT_DROP = drop_category());
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_DROP));
}

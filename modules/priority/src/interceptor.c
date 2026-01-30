#include "category.h"
#include <lotto/base/context.h>
#include <lotto/priority.h>
#include <lotto/runtime/intercept.h>
#include <lotto/util/once.h>

static category_t CAT_PRIORITY;

void
lotto_priority(int64_t priority)
{
    once(CAT_PRIORITY = priority_category());
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_PRIORITY,
                          .args = {arg(int64_t, priority)}));
}

void
lotto_priority_cond(bool cond, int64_t priority)
{
    if (!cond) {
        return;
    }
    lotto_priority(priority);
}

void
lotto_priority_task(task_id task, int64_t priority)
{
    if (get_task_id() != task) {
        return;
    }
    lotto_priority(priority);
}

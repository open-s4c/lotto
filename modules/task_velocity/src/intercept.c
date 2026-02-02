#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/assert.h>
#include <lotto/velocity.h>

void
lotto_task_velocity(int64_t probability)
{
    ASSERT(probability >= LOTTO_TASK_VELOCITY_MIN &&
           probability <= LOTTO_TASK_VELOCITY_MAX);

    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_TASK_VELOCITY,
                          .args = {arg(int64_t, probability)}));
}

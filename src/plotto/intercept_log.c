/*
 */

#include <lotto/log.h>
#include <lotto/runtime/intercept.h>

void
lotto_log(const uint64_t id)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_LOG_BEFORE));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_LOG_AFTER,
                          .args = {arg(uint64_t, id)}));
}

void
lotto_log_data(const uint64_t id, int64_t data)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_LOG_BEFORE));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_LOG_AFTER,
                          .args = {arg(uint64_t, id), arg(int64_t, data)}));
}

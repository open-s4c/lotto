#include <lotto/evec.h>
#include <lotto/runtime/intercept.h>

void
_lotto_evec_prepare(void *addr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_PREPARE,
                          .args = {arg_ptr(addr)}));
}

void
_lotto_evec_wait(void *addr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_WAIT,
                          .args = {arg_ptr(addr)}));
}

enum lotto_timed_wait_status
_lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    enum lotto_timed_wait_status ret;
    intercept_capture(
        ctx(.func = __FUNCTION__, .cat = CAT_EVEC_TIMED_WAIT,
            .args = {arg_ptr(addr), arg_ptr(abstime), arg_ptr(&ret)}));
    return ret;
}

void
_lotto_evec_cancel(void *addr)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_CANCEL,
                          .args = {arg_ptr(addr)}));
}

void
_lotto_evec_wake(void *addr, uint32_t cnt)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_WAKE,
                          .args = {arg_ptr(addr), arg(uint32_t, cnt)}));
}

void
_lotto_evec_move(void *src, void *dst)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SYS_YIELD));
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_EVEC_MOVE,
                          .args = {arg_ptr(src), arg_ptr(dst)}));
}

#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/evec.h>
#include <lotto/runtime/intercept.h>

void
_lotto_evec_prepare(void *addr)
{
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_prepare = ctx();
        ctx_prepare->func      = __FUNCTION__;
        ctx_prepare->cat       = CAT_EVEC_PREPARE;
        ctx_prepare->args[0] = arg_ptr(addr);
        intercept_capture(ctx_prepare);
    }
}

void
_lotto_evec_wait(void *addr)
{
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_wait = ctx();
        ctx_wait->func      = __FUNCTION__;
        ctx_wait->cat       = CAT_EVEC_WAIT;
        ctx_wait->args[0] = arg_ptr(addr);
        intercept_capture(ctx_wait);
    }
}

enum lotto_timed_wait_status
_lotto_evec_timed_wait(void *addr, const struct timespec *restrict abstime)
{
    enum lotto_timed_wait_status status = TIMED_WAIT_TIMEOUT;
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_wait = ctx();
        ctx_wait->func      = __FUNCTION__;
        ctx_wait->cat       = CAT_EVEC_TIMED_WAIT;
        ctx_wait->args[0] = arg_ptr(addr);
        ctx_wait->args[1] = arg_ptr(abstime);
        ctx_wait->args[2] = arg_ptr(&status);
        intercept_capture(ctx_wait);
    }
    return status;
}

void
_lotto_evec_cancel(void *addr)
{
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_cancel = ctx();
        ctx_cancel->func      = __FUNCTION__;
        ctx_cancel->cat       = CAT_EVEC_CANCEL;
        ctx_cancel->args[0] = arg_ptr(addr);
        intercept_capture(ctx_cancel);
    }
}

void
_lotto_evec_wake(void *addr, uint32_t cnt)
{
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_wake = ctx();
        ctx_wake->func      = __FUNCTION__;
        ctx_wake->cat       = CAT_EVEC_WAKE;
        ctx_wake->args[0] = arg_ptr(addr);
        ctx_wake->args[1] = arg(uint32_t, cnt);
        intercept_capture(ctx_wake);
    }
}

void
_lotto_evec_move(void *src, void *dst)
{
    if (intercept_capture != NULL) {
        context_t *yield_ctx = ctx();
        yield_ctx->func      = __FUNCTION__;
        yield_ctx->cat       = CAT_SYS_YIELD;
        intercept_capture(yield_ctx);
        context_t *ctx_move = ctx();
        ctx_move->func      = __FUNCTION__;
        ctx_move->cat       = CAT_EVEC_MOVE;
        ctx_move->args[0] = arg_ptr(src);
        ctx_move->args[1] = arg_ptr(dst);
        intercept_capture(ctx_move);
    }
}

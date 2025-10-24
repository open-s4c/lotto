/*
 */
#include <stdint.h>
#include <stdio.h>

#include <lotto/runtime/intercept.h>

extern void
__sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    static uint32_t N; // Counter for the guards.
    if (start == stop || *start)
        return; // Initialize only once.
    for (uint32_t *x = start; x < stop; x++)
        *x = ++N; // Guards should start from 1.
}

extern void
__sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    // intercept_capture(ctx(.func = "trace_pc", .cat = CAT_TRACE_PC,
    //                          .args = {arg_ptr(guard)}));
}

extern void
__sanitizer_cov_trace_pc(uint32_t *guard)
{
    // intercept_capture(ctx(.func = "trace_pc", .cat = CAT_TRACE_PC,
    //                          .args = {arg_ptr(guard)}));
}

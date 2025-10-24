/*
 */

#include <lotto/base/trace.h>
#include <lotto/base/trace_impl.h>
#include <lotto/sys/stdlib.h>
#include <lotto/util/redirect.h>

REDIRECT_RETURN(trace, int, append, (trace_t * t, record_t *r), t, t, r)
REDIRECT_RETURN(trace, int, append_safe, (trace_t * t, record_t *r), t, t, r)
REDIRECT_RETURN(trace, record_t *, next, (trace_t * t, enum record filter), t,
                t, filter)
REDIRECT_VOID(trace, advance, (trace_t * t), t, t)
REDIRECT_RETURN(trace, record_t *, last, (trace_t * t), t, t)
REDIRECT_RETURN(trace, stream_t *, stream, (trace_t * t), t, t)
REDIRECT_VOID(trace, forget, (trace_t * t), t, t)
REDIRECT_VOID(trace, clear, (trace_t * t), t, t)
REDIRECT_VOID(trace, load, (trace_t * t), t, t)
REDIRECT_VOID(trace, save, (const trace_t *t), t, t)
REDIRECT_VOID(trace, save_to, (const trace_t *t, stream_t *stream), t, t,
              stream)

void
trace_destroy(trace_t *t)
{
    if (t == NULL)
        return;
    trace_clear(t);
    sys_free(t);
}

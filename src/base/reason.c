/*
 */
#include <lotto/base/reason.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

#define GEN_REASON(reason) [REASON_##reason] = #reason,
static const char *_reason_map[] = {FOR_EACH_REASON};
#undef GEN_SLOT

BIT_STR(reason, REASON)

bool
reason_is_runtime(reason_t r)
{
    return REASON_RUNTIME(r);
}

bool
reason_is_shutdown(reason_t r)
{
    return IS_REASON_SHUTDOWN(r);
}

bool
reason_is_abort(reason_t r)
{
    return IS_REASON_ABORT(r);
}

bool
reason_is_terminate(reason_t r)
{
    return IS_REASON_TERMINATE(r);
}

bool
reason_is_record_final(reason_t r)
{
    return IS_REASON_RECORD_FINAL(r);
}

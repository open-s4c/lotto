/*
 */

#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

#define GEN_STOP_REASON(sr) [STOP_REASON_##sr] = #sr,
static const char *_stop_reason_map[] = {FOR_EACH_STOP_REASON};
#undef GEN_STOP_REASON

BIT_STR(stop_reason, STOP_REASON)

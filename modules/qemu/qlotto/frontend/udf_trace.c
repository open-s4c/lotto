/*
 */

#include <lotto/qemu/lotto_udf_trace.h>
#include <lotto/sys/assert.h>
#include <lotto/util/macros.h>

#define GEN_UDF_TRACE(udftrace) [UDF_TRACE_##udftrace] = #udftrace,
static const char *_udf_trace_map[] = {FOR_EACH_UDF_TRACE};
#undef GEN_TOPIC

BIT_STR(udf_trace, UDF_TRACE)

/*
 */
#ifndef LOTTO_UDF_TRACE_H
#define LOTTO_UDF_TRACE_H

#define FOR_EACH_UDF_TRACE                                                     \
    GEN_UDF_TRACE(EFI)                                                         \
    GEN_UDF_TRACE(INIT)                                                        \
    GEN_UDF_TRACE(ACPI)                                                        \
    GEN_UDF_TRACE(INIT_PROCESS)

#define GEN_UDF_TRACE(udftrace) UDF_TRACE_##udftrace,
typedef enum udf_trace {
    TOPIC_NONE = 0,
    FOR_EACH_UDF_TRACE UDF_TRACE_END_
} udf_trace_t;
#undef GEN_UDF_TRACE

/**
 * Returns a string representation of the udf trace name.
 */
const char *udf_trace_str(udf_trace_t t);

#endif // LOTTO_UDF_TRACE

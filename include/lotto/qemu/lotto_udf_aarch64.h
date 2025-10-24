/*
 */
#ifndef LOTTO_UDF_AARCH64_H
#define LOTTO_UDF_AARCH64_H

#include <lotto/qemu/lotto_udf_aarch64_generic.h>

/* hex words
    abba, acdc, babe, bad, bed, beef, cafe, c0de, dad, dead, deaf, deed, d0d0,
   face, fade, fed, feed, f00d
 */

// official udf instructions for aarch64 have 16 top most bits 0
// https://developer.arm.com/documentation/ddi0596/2021-12/Base-Instructions/UDF--Permanently-Undefined-?lang=en
#define UDF_AARCH64_INSTR_YIELD_VAL 0x0000cafe
#define UDF_AARCH64_INSTR_YIELD     AARCH64_INSTR0(UDF_AARCH64_INSTR_YIELD_VAL)

#define UDF_AARCH64_INSTR_SNAPSHOT_VAL 0x00000bed
#define UDF_AARCH64_INSTR_SNAPSHOT                                             \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_SNAPSHOT_VAL)

#define UDF_AARCH64_INSTR_QEMUQUIT_VAL 0x0000dead
#define UDF_AARCH64_INSTR_QEMUQUIT                                             \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_QEMUQUIT_VAL)

#define UDF_AARCH64_INSTR_LOCK_ACQ_VAL 0x0000affe
#define UDF_AARCH64_INSTR_LOCK_ACQ(lock)                                       \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_LOCK_ACQ_VAL, lock)

#define UDF_AARCH64_INSTR_LOCK_REL_VAL 0x0000effa
#define UDF_AARCH64_INSTR_LOCK_REL(lock)                                       \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_LOCK_REL_VAL, lock)

#define UDF_AARCH64_INSTR_LOCK_TRYACQ_VAL 0x0000af00
#define UDF_AARCH64_INSTR_LOCK_TRYACQ(lock)                                    \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_LOCK_TRYACQ_VAL, lock)

#define UDF_AARCH64_INSTR_LOCK_TRYACQ_SUCC_VAL 0x0000af11
#define UDF_AARCH64_INSTR_LOCK_TRYACQ_SUCC(lock)                               \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_LOCK_TRYACQ_SUCC_VAL, lock)

#define UDF_AARCH64_INSTR_LOCK_TRYACQ_FAIL_VAL 0x0000afff
#define UDF_AARCH64_INSTR_LOCK_TRYACQ_FAIL(lock)                               \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_LOCK_TRYACQ_FAIL_VAL, lock)

#define UDF_AARCH64_INSTR_REGION_IN_VAL 0x0000beef
#define UDF_AARCH64_INSTR_REGION_IN                                            \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_REGION_IN_VAL)

#define UDF_AARCH64_INSTR_REGION_OUT_VAL 0x0000feeb
#define UDF_AARCH64_INSTR_REGION_OUT                                           \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_REGION_OUT_VAL)

#define UDF_AARCH64_INSTR_TEST_SUCCESS_VAL 0x0000aaaa
#define UDF_AARCH64_INSTR_TEST_SUCCESS                                         \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_TEST_SUCCESS_VAL)

#define UDF_AARCH64_INSTR_TEST_FAIL_VAL 0x0000aaab
#define UDF_AARCH64_INSTR_TEST_FAIL                                            \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_TEST_FAIL_VAL)

#define UDF_AARCH64_INSTR_NO_INSTRUMENT_VAL 0x0000feed
#define UDF_AARCH64_INSTR_NO_INSTRUMENT                                        \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_NO_INSTRUMENT_VAL)

#define UDF_AARCH64_INSTR_TRACE_START_VAL 0x0000acdc
#define UDF_AARCH64_INSTR_TRACE_START(id)                                      \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_TRACE_START_VAL, id)

#define UDF_AARCH64_INSTR_TRACE_END_VAL 0x0000dcac
#define UDF_AARCH64_INSTR_TRACE_END(id)                                        \
    AARCH64_INSTR1(UDF_AARCH64_INSTR_TRACE_END_VAL, id)

#endif

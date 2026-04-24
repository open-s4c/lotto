/**
 * @file lotto_udf_aarch64.h
 * @brief QEMU guest declarations for lotto udf aarch64.
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
#define UDF_AARCH64_INSTR_LOTTO_TRAP_VAL 0x0000cafe
#define UDF_AARCH64_INSTR_LOTTO_TRAP(op)                                       \
    AARCH64_TRAP0(UDF_AARCH64_INSTR_LOTTO_TRAP_VAL, op)
#define UDF_AARCH64_INSTR_LOTTO_TRAP1(op, arg0)                                \
    AARCH64_TRAP1(UDF_AARCH64_INSTR_LOTTO_TRAP_VAL, op, arg0)

#define UDF_AARCH64_INSTR_EXIT_SNAPSHOT_VAL 0x00000bed
#define UDF_AARCH64_INSTR_EXIT_SNAPSHOT                                        \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_EXIT_SNAPSHOT_VAL)

#define UDF_AARCH64_INSTR_EXIT_SUCCESS_VAL 0x0000aaaa
#define UDF_AARCH64_INSTR_EXIT_SUCCESS                                         \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_EXIT_SUCCESS_VAL)

#define UDF_AARCH64_INSTR_EXIT_FAILURE_VAL 0x0000aaab
#define UDF_AARCH64_INSTR_EXIT_FAILURE                                         \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_EXIT_FAILURE_VAL)

#define UDF_AARCH64_INSTR_EXIT_ABANDON_VAL 0x0000aaac
#define UDF_AARCH64_INSTR_EXIT_ABANDON                                         \
    AARCH64_INSTR0(UDF_AARCH64_INSTR_EXIT_ABANDON_VAL)

#endif

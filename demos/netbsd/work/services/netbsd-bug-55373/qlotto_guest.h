#ifndef DEMOS_NETBSD_QLOTTO_GUEST_H
#define DEMOS_NETBSD_QLOTTO_GUEST_H

#include <stdint.h>
#include <stdlib.h>

typedef enum qlotto_bias_policy_kind {
    QLOTTO_BIAS_POLICY_NONE = 0,
    QLOTTO_BIAS_POLICY_CURRENT,
    QLOTTO_BIAS_POLICY_LOWEST,
    QLOTTO_BIAS_POLICY_HIGHEST,
} qlotto_bias_policy_t;

typedef enum qlotto_exit_kind {
    QLOTTO_EXIT_KIND_SUCCESS = 0,
    QLOTTO_EXIT_KIND_FAILURE = 1,
} qlotto_exit_kind_t;

#define QLOTTO_EVENT_TERMINATE_SUCCESS 183U
#define QLOTTO_EVENT_TERMINATE_FAILURE 184U
#define QLOTTO_EVENT_QEMU_SNAPSHOT     188U
#define QLOTTO_EVENT_BIAS_POLICY       190U

#if defined(__aarch64__)
#define QLOTTO_STR_(x) #x
#define QLOTTO_STR(x)  QLOTTO_STR_(x)

#define QLOTTO_REG_ARG1 20
#define QLOTTO_REG_ARG2 21

#define QLOTTO_UDF_TRAP_VAL 0x0000cafe

#define QLOTTO_A64_INSTR2(val, arg0, arg1)                                       \
    do {                                                                         \
        __asm__ __volatile__("mov x" QLOTTO_STR(QLOTTO_REG_ARG1) ", %0\n\t"     \
                             "mov x" QLOTTO_STR(QLOTTO_REG_ARG2) ", %1\n\t"     \
                             ".inst " QLOTTO_STR(val)                            \
                             :                                                   \
                             : "r"(arg0), "r"(arg1)                             \
                             : "x" QLOTTO_STR(QLOTTO_REG_ARG1),                  \
                               "x" QLOTTO_STR(QLOTTO_REG_ARG2));                 \
    } while (0)

#define QLOTTO_A64_INSTR1(val, arg0)                                             \
    do {                                                                         \
        __asm__ __volatile__("mov x" QLOTTO_STR(QLOTTO_REG_ARG1) ", %0\n\t"     \
                             ".inst " QLOTTO_STR(val)                            \
                             :                                                   \
                             : "r"(arg0)                                         \
                             : "x" QLOTTO_STR(QLOTTO_REG_ARG1));                 \
    } while (0)

#define QLOTTO_A64_TRAP0(op)          QLOTTO_A64_INSTR1(QLOTTO_UDF_TRAP_VAL, op)
#define QLOTTO_A64_TRAP1(op, arg0)    QLOTTO_A64_INSTR2(QLOTTO_UDF_TRAP_VAL, op, arg0)
#endif

static inline void
qlotto_bias_policy(qlotto_bias_policy_t policy)
{
#if defined(__aarch64__)
    QLOTTO_A64_TRAP1(QLOTTO_EVENT_BIAS_POLICY, (uint64_t)policy);
#else
    (void)policy;
#endif
}

static inline void
qlotto_snapshot(void)
{
#if defined(__aarch64__) && defined(QLOTTO_SNAPSHOT_ENABLED)
    QLOTTO_A64_TRAP0(QLOTTO_EVENT_QEMU_SNAPSHOT);
#endif
}

static inline void
qlotto_exit(qlotto_exit_kind_t kind)
{
#if defined(__aarch64__)
    switch (kind) {
        case QLOTTO_EXIT_KIND_SUCCESS:
            QLOTTO_A64_TRAP0(QLOTTO_EVENT_TERMINATE_SUCCESS);
            break;
        case QLOTTO_EXIT_KIND_FAILURE:
            QLOTTO_A64_TRAP0(QLOTTO_EVENT_TERMINATE_FAILURE);
            break;
    }
#endif
    exit(kind == QLOTTO_EXIT_KIND_SUCCESS ? 0 : 1);
}

#endif

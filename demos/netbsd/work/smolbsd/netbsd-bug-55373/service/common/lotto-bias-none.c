#include <stdint.h>

typedef enum bias_policy_kind {
    BIAS_POLICY_NONE = 0,
    BIAS_POLICY_CURRENT,
    BIAS_POLICY_LOWEST,
    BIAS_POLICY_HIGHEST,
} bias_policy_t;

#define EVENT_BIAS_POLICY 190U
#define LOTTO_TRAP_OPCODE 0x0000cafeU

static inline void
lotto_bias_policy(bias_policy_t policy)
{
    __asm__ __volatile__(
        "mov x20, %0\n\t"
        "mov x21, %1\n\t"
        ".inst %2\n\t"
        :
        : "r"((uint64_t)EVENT_BIAS_POLICY), "r"((uint64_t)policy),
          "i"(LOTTO_TRAP_OPCODE)
        : "x20", "x21");
}

int
main(void)
{
    lotto_bias_policy(BIAS_POLICY_NONE);
    return 0;
}

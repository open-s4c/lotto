/*
 */
#ifndef LOTTO_UDF_AARCH64_GENERIC_H
#define LOTTO_UDF_AARCH64_GENERIC_H

#ifndef STR_UDF
    #define STR_UDF(x) STRSTR_UDF(x)
#endif

#ifndef STRSTR_UDF
    #define STRSTR_UDF(x) #x
#endif

#define UDF_REG_ARG_1 20
#define UDF_REG_ARG_2 21
#define UDF_REG_ARG_3 22
#define UDF_REG_ARG_4 23
#define UDF_REG_ARG_5 24

#define AARCH64_UDF_MASK 0xFFFF0000

#define AARCH64_INSTR0(val) __asm__ __volatile__(".inst " STR_UDF(val));

#define AARCH64_INSTR1(val, arg0)                                                      \
    {                                                                                  \
        __asm__ __volatile__(                                                        \
                         "mov x" STR_UDF(UDF_REG_ARG_1) ", %0"    "\n\t"             \
                         ".inst " STR_UDF(val)                    "\n\t"             \
                         "# argument picked %0"                                      \
                         :                                                           \
                         : "r"(arg0)                                                 \
                         : "x" STR_UDF(UDF_REG_ARG_1)                                \
                         ); \
    }

#define AARCH64_INSTR2(val, arg0, arg1)                                                \
    {                                                                                  \
        __asm__ __volatile__(                                                        \
                         "mov x" STR_UDF(UDF_REG_ARG_1) ", %0"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_2) ", %1"                        "\n\t"                 \
                         ".inst " STR_UDF(val)               "\n\t"                  \
                         "# arguments picked %0 %1"                                  \
                         :                                                           \
                         : "r"(arg0), "r"(arg1)                                      \
                         : "x" STR_UDF(UDF_REG_ARG_1) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_2)                                \
                         ); \
    }

#define AARCH64_INSTR3(val, arg0, arg1, arg2)                                          \
    {                                                                                  \
        __asm__ __volatile__(                                                        \
                         "mov x" STR_UDF(UDF_REG_ARG_1) ", %0"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_2) ", %1"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_3) ", %2"                        "\n\t"                 \
                         ".inst " STR_UDF(val)               "\n\t"                  \
                         "# arguments picked %0 %1 %2"                               \
                         :                                                           \
                         : "r"(arg0), "r"(arg1) , "r"(arg2)                          \
                         : "x" STR_UDF(UDF_REG_ARG_1) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_2) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_3)                                \
                         ); \
    }

#define AARCH64_INSTR4(val, arg0, arg1, arg2, arg3)                                    \
    {                                                                                  \
        __asm__ __volatile__(                                                        \
                         "mov x" STR_UDF(UDF_REG_ARG_1) ", %0"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_2) ", %1"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_3) ", %2"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_4) ", %3"                        "\n\t"                 \
                         ".inst " STR_UDF(val)               "\n\t"                  \
                         "# arguments picked %0 %1 %2 %3"                            \
                         :                                                           \
                         : "r"(arg0), "r"(arg1) , "r"(arg2) , "r"(arg3)              \
                         : "x" STR_UDF(UDF_REG_ARG_1) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_2) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_3) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_4)                                \
                         ); \
    }

#define AARCH64_INSTR5(val, arg0, arg1, arg2, arg3, arg4)                              \
    {                                                                                  \
        __asm__ __volatile__(                                                        \
                         "mov x" STR_UDF(UDF_REG_ARG_1) ", %0"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_2) ", %1"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_3) ", %2"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_4) ", %3"                        "\n\t"                 \
                         "mov x" STR_UDF(UDF_REG_ARG_5) ", %4"                        "\n\t"                 \
                         ".inst " STR_UDF(val)               "\n\t"                  \
                         "# arguments picked %0 %1 %2 %3 %4"                         \
                         :                                                           \
                         : "r"(arg0), "r"(arg1) , "r"(arg2) , "r"(arg3) , "r"(arg4)  \
                         : "x" STR_UDF(UDF_REG_ARG_1) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_2) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_3) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_4) ,                              \
                           "x" STR_UDF(UDF_REG_ARG_5)                                \
                         ); \
    }

#endif // LOTTO_UDF_AARCH64_GENERIC_H

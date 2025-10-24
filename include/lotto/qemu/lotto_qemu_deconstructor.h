/*
 */
#ifndef LOTTO_QEMU_DECONSTRUCTOR_H
#define LOTTO_QEMU_DECONSTRUCTOR_H

#include <stdio.h>

#include <lotto/qemu/lotto_udf.h>

#if defined(QLOTTO_ENABLED) && defined(__ARM_ARCH)
    #define QLOTTO_DECONSTRUCTOR                                               \
        void __attribute__((destructor)) qlotto_exit(void);                    \
        void qlotto_exit(void)                                                 \
        {                                                                      \
            fprintf(stderr, "QLotto test exit deconstructor.\n");              \
            LOTTO_TEST_SUCCESS;                                                \
        }
#else
    #define QLOTTO_DECONSTRUCTOR
#endif

#endif // LOTTO_QEMU_DECONSTRUCTOR_H

#ifndef LOTTO_MODULES_AUTOCEPT_H
#define LOTTO_MODULES_AUTOCEPT_H

#ifndef __ASSEMBLER__
    #error "autocept.h is for preprocessed assembly (.S) only"
#endif

#if defined(__linux__)
// clang-format off
.section .note.GNU-stack,"",@progbits
// clang-format on
#endif

#if defined(__x86_64__)
    #include <lotto/modules/autocept/x86_64.S>
#elif defined(__aarch64__)
    #include <lotto/modules/autocept/aarch64.S>
#else
    #error "unsupported autocept architecture"
#endif

#include <lotto/modules/autocept/generic.S>

#define ADD_INTERCEPTOR(...) add_interceptor __VA_ARGS__

#endif

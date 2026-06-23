/**
 * @file _sys.h
 * @brief Unsafe declarations for  sys.
 */
#ifndef LOTTO_UNSAFE_SYS_H
#define LOTTO_UNSAFE_SYS_H

#include <lotto/sys/signatures/all.h>
#include <lotto/sys/signatures/defaults_wrap.h>

#if defined(__APPLE__)
    #ifdef memcpy
        #undef memcpy
    #endif
    #ifdef memset
        #undef memset
    #endif
    #ifdef memmove
        #undef memmove
    #endif
    #ifdef stpcpy
        #undef stpcpy
    #endif
    #ifdef strcpy
        #undef strcpy
    #endif
    #ifdef strcat
        #undef strcat
    #endif
    #ifdef sprintf
        #undef sprintf
    #endif
    #ifdef snprintf
        #undef snprintf
    #endif
    #ifdef vsprintf
        #undef vsprintf
    #endif
    #ifdef vsnprintf
        #undef vsnprintf
    #endif
    #ifdef sigaddset
        #undef sigaddset
    #endif
    #ifdef sigemptyset
        #undef sigemptyset
    #endif
#endif

#define PLF(i) PLF_ENABLE(i)
#define SLF(i) SLF_ENABLE(i)

#define SYS_FUNC(LIB, R, S, ATTR)   RL_FUNC_WRAP(LIB, R, S, ATTR)
#define SYS_FORMAT_FUNC(R, S, ATTR) RL_FORMAT_FUNC_WRAP(R, S, ATTR)

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-security"
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif
FOR_EACH_SYS_FUNC

#if defined(__clang__)
    #pragma clang diagnostic pop
#else
    #pragma GCC diagnostic pop
#endif
#undef SYS_FORMAT_FUNC
#undef SYS_FUNC

#undef PLF
#undef SLF

#endif // LOTTO_UNSAFE_SYS_H

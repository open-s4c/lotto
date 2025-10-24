/*
 */
#ifndef LOTTO_QEMU_SIGHANDLER_H
#define LOTTO_QEMU_SIGHANDLER_H

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <lotto/qemu/lotto_udf.h>

#if defined(QLOTTO_ENABLED) && defined(__ARM_ARCH)
    #define QLOTTO_SIGHANDLER                                                  \
        void sighandler_init(void);                                            \
        static void _handle_sigabrt(int sig, siginfo_t *si, void *arg);        \
        void __attribute__((constructor)) qlotto_siginit(void);                \
        void qlotto_siginit(void)                                              \
        {                                                                      \
            sighandler_init();                                                 \
        }                                                                      \
        static void _handle_sigabrt(int sig, siginfo_t *si, void *arg)         \
        {                                                                      \
            fprintf(stderr, "Caught SIGABRT in test sighandler.\n");           \
            LOTTO_TEST_FAIL;                                                   \
        }                                                                      \
        void sighandler_init(void)                                             \
        {                                                                      \
            fprintf(stderr, "Init SIGABRT sighandler.\n");                     \
            {                                                                  \
                struct sigaction action;                                       \
                memset(&action, 0, sizeof(struct sigaction));                  \
                action.sa_flags     = SA_SIGINFO;                              \
                action.sa_sigaction = _handle_sigabrt;                         \
                sigaction(SIGABRT, &action, NULL);                             \
            }                                                                  \
        }

#else
    #define QLOTTO_SIGHANDLER
#endif

#endif // LOTTO_QEMU_SIGHANDLER_H

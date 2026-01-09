/*
 */
#ifndef LOTTO_REAL_H
#define LOTTO_REAL_H

// NOLINTBEGIN(clang-diagnostic-unused-variable)

// clang-format off
#ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
#endif
#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
// clang-format on

#include <stdbool.h>
#include <stddef.h>

#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/util/macros.h>

#define REAL_STR(name)          #name
#define REAL_NAME(x)            CONCAT(__real_, x)
#define REAL_DECL(T, func, ...) static T (*REAL_NAME(func))(__VA_ARGS__)
#define REAL(func, ...)         REAL_NAME(func)(__VA_ARGS__)

#define REAL_LIBC    "libc.so.6"
#define REAL_PTHREAD "libpthread.so.0"

static inline void *
real_func_impl(const char *fname, const char *lib, bool try)
{
    Dl_info info;
    const size_t pthread_strlen = strlen("pthread_");

    ASSERT(NULL != fname);

    if (strlen(fname) > pthread_strlen &&
        strncmp(fname, "pthread_", pthread_strlen) == 0) {
        lib = REAL_PTHREAD;
    }
    if (lib == NULL) {
        void *func = dlsym(RTLD_NEXT, fname);

        if (func == NULL) {
            if (try)
                return NULL;
            logger_fatalf("could not find: %s",
                       dlerror()); // TODO: avoid infinite loop by not using
                                   // sys_ functions
        }

        if (0 == dladdr(func, &info)) {
            logger_fatalf("could not get DL_info struct for %s :%s", fname,
                       dlerror());
        }

        return func;
    }

    void *handle = dlopen(lib, RTLD_LAZY);
    if (handle == NULL) {
        if (try)
            return NULL;
        logger_fatalf("dlopen failed:%s", dlerror());
    }

    void *func = dlsym(handle, fname);
    if (func == NULL && !try) {
        logger_fatalf("could not find:%s", dlerror());
    }

    return func;
}

static inline void *
real_func(const char *fname, const char *lib)
{
    return real_func_impl(fname, lib, false);
}

static inline void *
real_func_try(const char *fname, const char *lib)
{
    return real_func_impl(fname, lib, true);
}

#define REAL_APPLY(A, ...) REAL_##A(__VA_ARGS__)
#define REAL_LIB_INIT(LIB, T, F, ...)                                          \
    REAL_APPLY(DECL, T, F, ##__VA_ARGS__);                                     \
    if (REAL_APPLY(NAME, F) == NULL) {                                         \
        REAL_APPLY(NAME, F) = real_func(REAL_APPLY(STR, F), LIB);              \
    }                                                                          \
    do {                                                                       \
    } while (0)

#define REAL_INIT(T, F, ...)      REAL_LIB_INIT(0, T, F, ##__VA_ARGS__)
#define REAL_LIBC_INIT(T, F, ...) REAL_LIB_INIT(REAL_LIBC, T, F, ##__VA_ARGS__)
#define REAL_PTHREAD_INIT(T, F, ...)                                           \
    REAL_LIB_INIT(REAL_PTHREAD, T, F, ##__VA_ARGS__)

// NOLINTEND(clang-diagnostic-unused-variable)

#endif /* LOTTO_REAL_H */

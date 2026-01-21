/*
 */
#ifndef LOTTO_CLI_PRELOAD_H
#define LOTTO_CLI_PRELOAD_H
#include <stdbool.h>

#include <lotto/base/libraries.h>

#define LIBTSANO         "libtsano.so"
#define LIBTSAN0         "libtsan.so.0"
#define LIBTSAN2         "libtsan.so.2"
#define LIBCLANG_RT_TSAN "libclang_rt.tsan-x86_64.so"

#define GDB            "debug.gdb"
#define GDB_PYTHON_DIR "pylotto"

void preload(const char *path, bool verbose, bool plotto,
             const char *memmgr_chain_runtime, const char *memmgr_chain_user);
void debug_preload(const char *dir, const char *file_filter,
                   const char *function_filter, const char *addr2line,
                   const char *symbol_file);

void preload_set_libpath(const char *path);

#endif

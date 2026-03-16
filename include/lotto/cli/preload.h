/**
 * @file preload.h
 * @brief CLI declarations for preload.
 */
#ifndef LOTTO_CLI_PRELOAD_H
#define LOTTO_CLI_PRELOAD_H

#include <stdbool.h>

void preload(const char *path, bool verbose, bool plotto,
             const char *memmgr_chain_runtime, const char *memmgr_chain_user);

void preload_set_libpath(const char *path);

#endif

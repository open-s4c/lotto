/**
 * @file preload.h
 * @brief Driver declarations for preload.
 */
#ifndef LOTTO_DRIVER_PRELOAD_H
#define LOTTO_DRIVER_PRELOAD_H

#include <stdbool.h>
#include <stdint.h>

void preload(const char *path, uint64_t verbose, bool plotto,
             const char *memmgr_chain_runtime, const char *memmgr_chain_user);

void preload_set_libpath(const char *path);

#endif

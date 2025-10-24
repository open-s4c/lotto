/*
 */
#ifndef LOTTO_MEMMGR_RUNTIME_H
#define LOTTO_MEMMGR_RUNTIME_H

#include <stddef.h>

#include <lotto/util/macros.h>

void *memmgr_runtime_alloc(size_t size);
void *memmgr_runtime_aligned_alloc(size_t alignment, size_t size) WEAK;
void *memmgr_runtime_realloc(void *ptr, size_t size) WEAK;
void memmgr_runtime_free(void *ptr);
void memmgr_runtime_fini() WEAK;

#endif

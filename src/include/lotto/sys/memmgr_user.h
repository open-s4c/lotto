/*
 */
#ifndef LOTTO_MEMMGR_USER_H
#define LOTTO_MEMMGR_USER_H

#include <stddef.h>

#include <lotto/util/macros.h>

void *memmgr_user_alloc(size_t size);
void *memmgr_user_aligned_alloc(size_t alignment, size_t size);
void *memmgr_user_realloc(void *ptr, size_t size);
void memmgr_user_free(void *ptr);
void memmgr_user_fini();

#endif

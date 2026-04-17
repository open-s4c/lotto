/**
 * @file memmgr_user.h
 * @brief System wrapper declarations for memmgr user.
 */
#ifndef LOTTO_MEMMGR_USER_H
#define LOTTO_MEMMGR_USER_H

#include <stddef.h>

#include <lotto/util/macros.h>

void *memmgr_user_alloc(size_t size) WEAK;
void *memmgr_user_aligned_alloc(size_t alignment, size_t size) WEAK;
void *memmgr_user_realloc(void *ptr, size_t size) WEAK;
void memmgr_user_free(void *ptr) WEAK;
void memmgr_user_fini() WEAK;

#endif

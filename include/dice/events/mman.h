/*
 * Copyright (C) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: 0BSD
 */
#ifndef DICE_MMAN_H
#define DICE_MMAN_H

#include <stddef.h>

#include <sys/mman.h>

#define EVENT_MMAP   80
#define EVENT_MUNMAP 81

struct mmap_event {
    const void *pc;
    void *addr;
    size_t length;
    int prot;
    int flags;
    int fd;
    off_t offset;
    void *ret;
};

struct munmap_event {
    const void *pc;
    void *addr;
    size_t length;
    int ret;
};

#endif /* DICE_MMAN_H */

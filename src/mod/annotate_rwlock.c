/*
 * Copyright (C) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: 0BSD
 */
#include <pthread.h>

#include <dice/chains/intercept.h>
#include <dice/events/annotate_rwlock.h>
#include <dice/interpose.h>
#include <dice/pubsub.h>


INTERPOSE(void, AnnotateRWLockCreate, const char *file, int line,
          const volatile void *lock)
{
    struct AnnotateRWLockCreate_event ev = {
        .file = file,
        .line = line,
        .lock = lock,
    };

    metadata_t md = {0};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ANNOTATE_RWLOCK_CREATE, &ev, &md);
}

INTERPOSE(void, AnnotateRWLockDestroy, const char *file, int line,
          const volatile void *lock)
{
    struct AnnotateRWLockDestroy_event ev = {
        .file = file,
        .line = line,
        .lock = lock,
    };

    metadata_t md = {0};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ANNOTATE_RWLOCK_DESTROY, &ev, &md);
}

INTERPOSE(void, AnnotateRWLockAcquired, const char *file, int line,
          const volatile void *lock, long is_w)
{
    struct AnnotateRWLockAcquired_event ev = {.file = file,
                                              .line = line,
                                              .lock = lock,
                                              .is_w = is_w};

    metadata_t md = {0};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ANNOTATE_RWLOCK_ACQ, &ev, &md);
}

INTERPOSE(void, AnnotateRWLockReleased, const char *file, int line,
          const volatile void *lock, long is_w)
{
    struct AnnotateRWLockReleased_event ev = {.file = file,
                                              .line = line,
                                              .lock = lock,
                                              .is_w = is_w};

    metadata_t md = {0};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_ANNOTATE_RWLOCK_REL, &ev, &md);
}

DICE_MODULE_INIT()

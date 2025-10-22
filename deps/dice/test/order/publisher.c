/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * SPDX-License-Identifier: 0BSD
 */
#include "defs.h"
#include <dice/module.h>
#include <dice/pubsub.h>

DICE_MODULE_INIT()

void
publish(struct event *ev)
{
    (void)ev;
    PS_PUBLISH(CHAIN, EVENT, ev, 0);
}

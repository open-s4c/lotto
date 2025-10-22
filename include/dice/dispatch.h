/*
 * Copyright (C) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: 0BSD
 */
#ifndef DICE_DISPATCH_H
#define DICE_DISPATCH_H
#include <dice/compiler.h>
#include <dice/pubsub.h>

#define PS_DISPATCH(CHAIN, TYPE, SLOT)                                         \
    V_JOIN(V_JOIN(ps_dispatch, V_JOIN(CHAIN, V_JOIN(TYPE, SLOT))), )

#define PS_DISPATCH_DECL(CHAIN, TYPE, SLOT)                                    \
    DICE_HIDE enum ps_err PS_DISPATCH(CHAIN, TYPE, SLOT)(                      \
        const chain_id chain, const type_id type, void *event, metadata_t *md) \
    {                                                                          \
        return PS_HANDLER(CHAIN, TYPE, SLOT)(chain, type, event, md);          \
    }

DICE_HIDE bool
V_JOIN(V_JOIN(ps_dispatch_slot, DICE_MODULE_PRIO), on_)(void)
{
    return true;
}

#endif /* DICE_DISPATCH_H */

/*
 */

#ifndef LOTTO_QEMU_EVENT_H
#define LOTTO_QEMU_EVENT_H

#include <stdint.h>

#include <lotto/base/category.h>
#include <lotto/base/map.h>

typedef struct event_s {
    mapitem_t ti;
    category_t cat;
    char *func_name;
} eventi_t;

typedef struct event_context_s {
    context_t *ctx;
    eventi_t *event;
} event_context_t;

void qlotto_add_event(map_t *emap, uint64_t e_pc, category_t cat,
                      char *func_name);
void qlotto_del_event(map_t *emap, uint64_t e_pc);
eventi_t *qlotto_get_event(map_t *emap, uint64_t e_pc);


#endif // LOTTO_QEMU_EVENT_H

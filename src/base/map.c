/*
 */
#include <stddef.h>
#include <string.h>

#include <lotto/base/map.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define MARSHABLE                                                              \
    (marshable_t)                                                              \
    {                                                                          \
        .marshal = _marshal, .unmarshal = _unmarshal, .size = _size,           \
    }

static size_t
_size(const marshable_t *m)
{
    ASSERT(m);
    map_t *p       = (map_t *)m;
    size_t s       = sizeof(map_t);
    mapitem_t *cur = p->tail;
    for (; cur; cur = cur->next)
        s += marshable_size_m(cur);
    return s;
}


static void *
_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b    = (char *)buf;
    map_t *map = (map_t *)m;

    sys_memcpy(b, map, sizeof(map_t));
    b += sizeof(map_t);

    mapitem_t *cur = map->tail;
    for (; cur; cur = cur->next) {
        b = marshable_marshal_m(cur, b);
    }
    return b;
}


static const void *
_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    const char *b = (const char *)buf;
    size_t off    = offsetof(map_t, payload);
    map_t *map    = (map_t *)m;

    /* remove any remaining items */
    map_clear(map);

    /* unmarshal non-task state */
    sys_memcpy(map->payload, b + off, sizeof(map_t) - off);
    b += sizeof(map_t);

    /* unmarshal tasks */
    size_t nitems = map->nitems;
    map->nitems   = 0;
    map->tail     = NULL;

    for (size_t i = 0; i < nitems; i++) {
        mapitem_t *prev = map->tail;
        mapitem_t *cur  = (mapitem_t *)b;
        cur             = map_register(map, cur->key);
        b               = marshable_unmarshal_m(cur, b);
        cur->next       = prev;
    }

    return b;
}

void
map_init(map_t *map, marshable_t mi)
{
    ASSERT(map);
    ASSERT(mi.alloc_size >= sizeof(mapitem_t));

    *map = (map_t){
        .m      = MARSHABLE,
        .mi     = mi,
        .nitems = 0,
        .tail   = NULL,
    };
}

mapitem_t *
map_find(const map_t *map, uint64_t key)
{
    ASSERT(map);
    mapitem_t *cur = map->tail;
    for (; cur && cur->key != key; cur = cur->next)
        ;
    return cur;
}

mapitem_t *
map_find_first(const map_t *map, bool(predicate)(const mapitem_t *))
{
    ASSERT(map);
    mapitem_t *cur = map->tail;
    for (; cur && !predicate(cur); cur = cur->next)
        ;
    return cur;
}

mapitem_t *
map_register(map_t *map, uint64_t key)
{
    ASSERT(map);
    ASSERT(map_find(map, key) == NULL);

    mapitem_t *i = (mapitem_t *)sys_calloc(1, map->mi.alloc_size);
    ASSERT(i);

    i->key    = key;
    i->m      = map->mi;
    i->next   = map->tail;
    map->tail = i;
    map->nitems++;

    return i;
}

void
map_deregister(map_t *map, uint64_t key)
{
    ASSERT(map);

    mapitem_t *i = map_find(map, key);
    if (i == NULL)
        return;

    map->nitems--;

    if (map->tail == i) {
        map->tail = i->next;
    } else {
        mapitem_t *prev = map->tail;
        for (; prev->next != i; prev = prev->next)
            ;
        prev->next = i->next;
    }
    sys_free(i);
}

size_t
map_size(const map_t *map)
{
    return map->nitems;
}

const mapitem_t *
map_iterate(const map_t *map)
{
    ASSERT(map);
    return map->tail;
}

const mapitem_t *
map_next(const mapitem_t *mi)
{
    return mi->next;
}

void
map_clear(map_t *map)
{
    while (map->tail) {
        mapitem_t *t = map->tail->next;
        sys_free(map->tail);
        map->tail = t;
    }
    map->nitems = 0;
}

mapitem_t *
map_find_or_register(map_t *map, uint64_t key, mapitem_init_f init)
{
    mapitem_t *result = map_find(map, key);
    if (result == NULL) {
        result = map_register(map, key);
    }
    if (init != NULL) {
        init(result);
    }
    return result;
}

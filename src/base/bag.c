/*
 */
#include <stddef.h>
#include <string.h>

#include <lotto/base/bag.h>
#include <lotto/base/marshable.h>
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
    bag_t *p       = (bag_t *)m;
    size_t s       = sizeof(bag_t);
    bagitem_t *cur = p->tail;
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
    bag_t *bag = (bag_t *)m;

    sys_memcpy(b, bag, sizeof(bag_t));
    b += sizeof(bag_t);

    bagitem_t *cur = bag->tail;
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
    size_t off    = offsetof(bag_t, payload);
    bag_t *bag    = (bag_t *)m;

    /* free existing contents */
    bag_clear(bag);

    /* unmarshal non-task state */
    sys_memcpy(bag->payload, b + off, sizeof(bag_t) - off);
    b += sizeof(bag_t);

    /* unmarshal tasks */
    bagitem_t *cur  = NULL;
    bagitem_t *prev = NULL;
    size_t nitems   = bag->nitems;
    bag->nitems     = 0;
    bag->tail       = NULL;

    for (size_t i = 0; i < nitems; i++) {
        prev      = bag->tail;
        cur       = bag_add(bag);
        b         = marshable_unmarshal_m(cur, b);
        cur->next = prev;
    }

    return b;
}

void
bag_init(bag_t *bag, marshable_t mi)
{
    ASSERT(bag);
    ASSERT(mi.alloc_size >= sizeof(bagitem_t));

    *bag = (bag_t){
        .m      = MARSHABLE,
        .mi     = mi,
        .nitems = 0,
        .tail   = NULL,
    };
}

bagitem_t *
bag_add(bag_t *bag)
{
    ASSERT(bag);
    bagitem_t *t = (bagitem_t *)sys_calloc(1, bag->mi.alloc_size);
    ASSERT(t);

    t->m      = bag->mi;
    t->next   = bag->tail;
    bag->tail = t;
    bag->nitems++;

    return t;
}

void
bag_remove(bag_t *b, bagitem_t *i)
{
    ASSERT(b);
    ASSERT(i);
    const bagitem_t *it = bag_iterate(b);
    for (; it; it = bag_next(it)) {
        if (it != i)
            continue;

        b->nitems--;
        if (b->tail == i) {
            b->tail = i->next;
        } else {
            bagitem_t *prev = b->tail;
            for (; prev->next != i; prev = prev->next)
                ;
            prev->next = i->next;
        }
        sys_free(i);
    }
}

size_t
bag_size(const bag_t *b)
{
    return b->nitems;
}

const bagitem_t *
bag_iterate(const bag_t *b)
{
    ASSERT(b);
    return b->tail;
}

const bagitem_t *
bag_next(const bagitem_t *it)
{
    return it->next;
}

void
bag_clear(bag_t *b)
{
    while (b->tail) {
        bagitem_t *t = b->tail->next;
        sys_free(b->tail);
        b->tail = t;
    }
    b->nitems = 0;
}

void
bag_copy(bag_t *dst, const bag_t *src)
{
    const bagitem_t *it = bag_iterate(src);
    for (; it; it = bag_next(it)) {
        bagitem_t *j = bag_add(dst);
        sys_memcpy(j->payload, it->payload,
                   it->m.alloc_size - sizeof(bagitem_t));
    }
}

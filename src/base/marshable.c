#include <lotto/base/marshable.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

size_t
marshable_size(const marshable_t *m)
{
    ASSERT(m);
    ASSERT(m->size);
    return m->size(m) + (m->next ? marshable_size(m->next) : 0);
}

void *
marshable_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(m->marshal);
    void *ptr = m->marshal(m, buf);
    return m->next ? marshable_marshal(m->next, ptr) : ptr;
}

const void *
marshable_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(m->unmarshal);
    const void *ptr = m->unmarshal(m, buf);
    return m->next ? marshable_unmarshal(m->next, ptr) : ptr;
}

void
marshable_print(marshable_t *m)
{
    ASSERT(m);
    if (m->print)
        m->print(m);
    if (m->next)
        marshable_print(m->next);
}

void
marshable_copy(marshable_t *dst, const marshable_t *src)
{
    ASSERT(src);
    ASSERT(dst);
    char *buf = sys_malloc(marshable_size(src));
    marshable_marshal(src, buf);
    marshable_unmarshal(dst, buf);
    sys_free(buf);
}

void
marshable_bind(marshable_t *parent, marshable_t *child)
{
    ASSERT(parent);
    ASSERT(child);
    child->next = NULL;
    while (parent->next)
        parent = parent->next;
    parent->next = child;
}

size_t
marshable_static_size(const marshable_t *m)
{
    return m->alloc_size;
}

void *
marshable_static_marshal(const marshable_t *m, void *buf)
{
    sys_memcpy(buf, m, marshable_static_size(m));
    return (char *)buf + marshable_static_size(m);
}

const void *
marshable_static_unmarshal(marshable_t *m, const void *buf)

{
    size_t off = offsetof(marshable_t, payload);
    sys_memcpy(m->payload, buf + off, marshable_static_size(m) - off);
    return (char *)buf + marshable_static_size(m);
}

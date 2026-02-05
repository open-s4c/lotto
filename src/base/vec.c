#include <stddef.h>
#include <string.h>

#include <lotto/base/vec.h>
#include <lotto/base/marshable.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

#define MARSHABLE                                                              \
    (marshable_t)                                                              \
    {                                                                          \
        .marshal = _marshal, .unmarshal = _unmarshal, .size = _size,           \
    }

#define AT(v,i) (vecitem_t *)((char *)(v)->items + (v)->mi.alloc_size * (i))

static size_t
_size(const marshable_t *m)
{
    ASSERT(m);
    vec_t *v = (vec_t *)m;
    size_t s = sizeof(size_t);
    if (!v->cnt)
        return s;
    for (size_t i = 0; i < v->cnt; ++i) {
        s += marshable_size_m(AT(v, i));
    }
    return s;
}


static void *
_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b  = (char *)buf;
    vec_t *v = (vec_t *)m;
    sys_memcpy(b, &v->cnt, sizeof(size_t));
    b += sizeof(size_t);
    for (size_t i = 0; i < v->cnt; ++i) {
        b = marshable_marshal_m(AT(v, i), b);
    }
    return b;
}


static const void *
_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    const char *b = (const char *)buf;
    vec_t *v      = (vec_t *)m;

    /* free existing contents */
    vec_clear(v);

    /* unmarshal header */
    size_t cnt;
    sys_memcpy(&cnt, b, sizeof(size_t));
    b += sizeof(size_t);

    /* unmarshal data */
    for (size_t i = 0; i < cnt; ++i) {
        vecitem_t *it = vec_add(v);
        b = marshable_unmarshal_m(it, b);
    }
    ASSERT(v->cnt == cnt);

    return b;
}

void
vec_init(vec_t *v, marshable_t mi)
{
    ASSERT(v);
    ASSERT(mi.alloc_size >= sizeof(vecitem_t));

    *v = (vec_t) {
        .m     = MARSHABLE,
        .mi    = mi,
        .cnt   = 0,
        .cap   = 0,
        .items = NULL,
    };
}


vecitem_t *
vec_add(vec_t *v)
{
    ASSERT(v);
    if (v->cnt >= v->cap) {
        v->cap = (v->cap == 0) ? 8 : v->cap * 2;
        v->items = sys_realloc(v->items, v->mi.alloc_size * v->cap);
        ASSERT(v->items);
    }
    size_t i = v->cnt;
    vecitem_t *it = AT(v, i);
    it->m = v->mi;
    v->cnt++;
    return it;
}


void
vec_clear(vec_t *v)
{
    ASSERT(v);
    sys_free(v->items);
    v->items = NULL;
    v->cap = 0;
    v->cnt = 0;
}


void
vec_sort(vec_t *v, vecitem_comparator_f compar)
{
    ASSERT(v);
    ASSERT(compar);
    typedef int(*qsort_compar)(const void *, const void *);
    qsort(v->items, v->cnt, v->mi.alloc_size, (qsort_compar)compar);
}


void
vec_reserve(vec_t *v, size_t cap)
{
    ASSERT(v);
    if (cap <= v->cap) {
        return;
    }
    v->cap = cap;
    v->items = sys_realloc(v->items, v->cap * v->mi.alloc_size);
    ASSERT(v->items);
}


void
vec_concat(vec_t *left, const vec_t *right)
{
    ASSERT(left);
    ASSERT(right);
    ASSERT(left->mi.alloc_size == right->mi.alloc_size);
    vec_reserve(left, left->cnt + right->cnt);
    sys_memcpy(AT(left, left->cnt), AT(right, 0), right->cnt * left->mi.alloc_size);
    left->cnt = left->cnt + right->cnt;
}


size_t
vec_size(const vec_t *v)
{
    ASSERT(v);
    return v->cnt;
}


vecitem_t *
vec_get(vec_t *v, size_t i)
{
    ASSERT(v);
    ASSERT(i < v->cnt);
    return AT(v, i);
}


void
vec_union(vec_t *left, const vec_t *right, vecitem_comparator_f compar)
{
    ASSERT(left);
    ASSERT(right);
    ASSERT(compar);

    vec_concat(left, right);
    vec_sort(left, compar);

    size_t j = 0;               /* next write pos */
    /* invariant: 0 <= j <= i <= left->cnt */
    for (size_t i = 0; i < left->cnt; ++i) {
        if (j == 0 || compar(AT(left, i), AT(left, j - 1)) != 0) {
            if (j != i) {
                sys_memcpy(AT(left, j), AT(left, i), left->mi.alloc_size);
            }
            j++;
        }
    }
    left->cnt = j;
}

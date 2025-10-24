#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/marshable.h>
#include <lotto/engine/pool.h>

typedef struct {
    tstate_t t;
    char msg[256];
} my_task_t;

#define MARSHABLE                                                              \
    (marshable_t)                                                              \
    {                                                                          \
        .marshal = _marshal, .unmarshal = _unmarshal, .size = _size,           \
    }

size_t
_size(const marshable_t *m)
{
    return sizeof(my_task_t);
}

void *
_marshal(const marshable_t *m, void *buf)
{
    memcpy(buf, m, _size(m));
    return (char *)buf + _size(m);
}

const void *
_unmarshal(marshable_t *m, const void *buf)
{
    memcpy(m, buf, _size(m));
    return (char *)buf + _size(m);
    *m = MARSHABLE;
    return buf;
}

void
test_task_marshaling()
{
    my_task_t t = {
        .t =
            {
                .m  = MARSHABLE,
                .id = 123,
            },
        .msg = "test",
    };

    size_t s = marshable_size_m(_m & t.t.m);
    assert(s == sizeof(my_task_t));

    char *buf = (char *)malloc(s);
    assert(buf);

    char *b2 = (char *)marshable_marshal_m(_m & t.t.m, buf);
    assert(buf + s == b2);

    /* since the marshaled data is the same as unmarshaled */
    my_task_t *t2 = (my_task_t *)buf;
    assert(strcmp(t2->msg, t.msg) == 0);

    my_task_t t3 = {0};
    t3.t.m       = MARSHABLE;

    char *b3 = (char *)marshable_unmarshal_m(_m & t3.t.m, buf);
    assert(buf + s == b3);

    assert(strcmp(t3.msg, t.msg) == 0);
    assert(t3.t.id == t.t.id);
}

int
main()
{
    test_task_marshaling();
    return 0;
}

#include <stdlib.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidset.h>
#include <lotto/sys/ensure.h>

struct data {
    marshable_t m;
    int something;
    tidset_t set;
};

void
test_size(void)
{
    struct data d;

    // initialize d, but leave tidset uninitialized
    d.m = MARSHABLE_STATIC(sizeof(struct data));
    ENSURE(marshable_size_m(&d) == sizeof(struct data));

    // initialize tidset and link data structures
    tidset_init(&d.set);
    d.m.next  = &d.set.m;
    size_t s1 = marshable_size_m(&d.set);
    ENSURE(marshable_size_m(&d) ==
           sizeof(struct data) + marshable_size_m(&d.set));

    for (size_t i = 0; i < 100; i++)
        tidset_insert(&d.set, i);
    ENSURE(marshable_size_m(&d) ==
           sizeof(struct data) + marshable_size_m(&d.set));
    ENSURE(s1 != marshable_size_m(&d.set));
}

void
test_marshal(void)
{
    struct data src;
    src.m = MARSHABLE_STATIC(sizeof(struct data));
    tidset_init(&src.set);
    src.m.next = &src.set.m;
    for (size_t i = 0; i < 50; i++)
        tidset_insert(&src.set, i);
    struct data dst;
    dst.m = MARSHABLE_STATIC(sizeof(struct data));
    marshable_copy_m(&dst, &src);

    ENSURE(marshable_size_m(&dst.set) == marshable_size_m(&src.set));

    ENSURE(dst.m.next == NULL);
    marshable_bind_m(&dst, &src);
    ENSURE(dst.m.next == &src.m);
}

int
main(void)
{
    test_size();
    test_marshal();
    return 0;
}

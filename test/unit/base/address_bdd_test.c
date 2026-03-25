#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lotto/base/address_bdd.h>

#define min(x, y) ((x) < (y) ? (x) : (y))

uint64_t
rand64()
{
    return (((uint64_t)rand()) << 32) + rand();
}

void
initialization_test()
{
    // NOLINTNEXTLINE(bugprone-narrowing-conversions)
    uint8_t max_depth  = rand64() % 64 + 1;
    address_bdd_t *bdd = address_bdd_new(max_depth);
    assert(!address_bdd_get_and(bdd, (void *)rand64(), rand64()));
    assert(!address_bdd_get_or(bdd, (void *)rand64(), rand64()));
    address_bdd_free(bdd);
}

void
assignment_test()
{
    // NOLINTNEXTLINE(bugprone-narrowing-conversions)
    uint8_t max_depth  = rand64() % 64 + 1;
    address_bdd_t *bdd = address_bdd_new(max_depth);
    uintptr_t addr     = rand64();
    uint64_t size =
        rand64() %
            ((((((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - 1)) - 1) << 1) +
             1 - addr) +
        1;
    assert(addr + size >= addr);
    address_bdd_set(bdd, (void *)addr, size);
    assert(address_bdd_get_and(bdd, (void *)addr, size));
    assert(address_bdd_get_or(bdd, (void *)addr, size));
    address_bdd_free(bdd);
}

void
overapproximation_test()
{
    // NOLINTNEXTLINE(bugprone-narrowing-conversions)
    uint8_t max_depth  = rand64() % 64 + 1;
    address_bdd_t *bdd = address_bdd_new(max_depth);
    uintptr_t addr     = rand64();
    uint64_t size =
        rand64() %
            ((((((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - 1)) - 1) << 1) +
             1 - addr) +
        1;
    assert(addr + size >= addr);
    address_bdd_set(bdd, (void *)addr, size);
    uintptr_t unit = ((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - max_depth);
    uintptr_t mask = ~(unit - 1);
    uint64_t start_addr = addr & mask;
    uint64_t end_addr   = ((uint64_t)(addr + size - 1) & mask) + unit - 1;
    uint64_t approximated_size = end_addr - start_addr + 1;
    assert(address_bdd_get_and(bdd, (void *)start_addr, approximated_size));
    assert(address_bdd_get_or(bdd, (void *)start_addr, approximated_size));
    if (start_addr - unit < start_addr) {
        assert(!address_bdd_get_and(bdd, (void *)start_addr - unit,
                                    approximated_size + unit ?
                                        approximated_size + unit :
                                        approximated_size + unit - 1));
        assert(address_bdd_get_or(bdd, (void *)start_addr - unit,
                                  approximated_size + unit));
    }
    if (end_addr + unit - 1 > end_addr) {
        assert(!address_bdd_get_and(bdd, (void *)start_addr,
                                    approximated_size + unit ?
                                        approximated_size + unit :
                                        approximated_size + unit - 1));
        assert(address_bdd_get_or(bdd, (void *)start_addr,
                                  approximated_size + unit ?
                                      approximated_size + unit :
                                      approximated_size + unit - 1));
    }
    address_bdd_free(bdd);
}

void
iterator_test()
{
    uint8_t max_depth =
        rand64() % 4 + 60; // depth at least 60 for performance reasons
    address_bdd_t *bdd = address_bdd_new(max_depth);
    uintptr_t addr     = rand64();
    uint64_t size =
        rand64() % min((((((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - 1)) - 1)
                        << 1) +
                           1 - addr,
                       1024) +
        1; // size at most 1024 for performance reasons
    assert(addr + size >= addr);
    address_bdd_set(bdd, (void *)addr, size);
    uintptr_t unit = ((uintptr_t)1) << ((sizeof(uintptr_t) << 3) - max_depth);
    uintptr_t mask = ~(unit - 1);
    uint64_t start_addr = addr & mask;
    uint64_t end_addr   = ((uint64_t)(addr + size - 1) & mask) + unit - 1;
    address_bdd_iterator_t *iterator = address_bdd_iterator(bdd);
    for (uintptr_t i = start_addr; i <= end_addr; i++) {
        assert(address_bdd_iterator_next(iterator) == i);
    }
    assert(!address_bdd_iterator_next(iterator));
    address_bdd_iterator_free(iterator);
    address_bdd_free(bdd);
}

int
main()
{
    void (*tests[])() = {initialization_test, assignment_test,
                         overapproximation_test, iterator_test};
    unsigned int seed = (unsigned int)time(NULL);
    printf("Seed: %d\n", seed);
    srand(seed);
    for (size_t i = 0; i < sizeof tests / sizeof(void (*)(void)); i++) {
        tests[i]();
    }
    return 0;
}

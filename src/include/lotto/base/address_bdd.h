/*
 */
#ifndef ADDRESS_BDD_H
#define ADDRESS_BDD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct address_bdd_s address_bdd_t;
typedef struct address_bdd_node_s address_bdd_node_t;
typedef struct address_bdd_iterator_s address_bdd_iterator_t;

void address_bdd_set(address_bdd_t *bdd, void *addr, size_t size);
void address_bdd_unset(address_bdd_t *bdd, void *addr, size_t size);
bool address_bdd_get_and(address_bdd_t *bdd, void *addr, size_t size);
bool address_bdd_get_or(address_bdd_t *bdd, void *addr, size_t size);
address_bdd_t *address_bdd_new(uint8_t max_depth);
void address_bdd_free(address_bdd_t *bdd);
void address_bdd_print(address_bdd_t *bdd);
void *address_bdd_get_masked(address_bdd_t *bdd, void *addr, uint64_t mask);
address_bdd_iterator_t *address_bdd_iterator(address_bdd_t *bdd);
uintptr_t address_bdd_iterator_next(address_bdd_iterator_t *iterator);
void address_bdd_iterator_free(address_bdd_iterator_t *iterator);
size_t address_bdd_size(address_bdd_t *bdd);

#endif

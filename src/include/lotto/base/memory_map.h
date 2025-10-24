/*
 */
#ifndef LOTTO_MEMORY_MAP_H
#define LOTTO_MEMORY_MAP_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char name[PATH_MAX];
    uint64_t offset;
} map_address_t;

typedef struct {
    char name[PATH_MAX];
    uint64_t offset;
    uint64_t address_start;
    uint64_t address_end;
} memory_map_entry_t;

memory_map_entry_t *memory_map_get(const char *path);
void memory_map_address_lookup(uintptr_t address, map_address_t *result);
void memory_map_address_lookup_from_path(uintptr_t address,
                                         map_address_t *result,
                                         const char *path);
bool memory_map_address_equals(const map_address_t *address1,
                               const map_address_t *address2);
int memory_map_address_sprint(const map_address_t *address, char *output);

#endif

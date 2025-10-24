/*
 */
#ifndef STRING_HASH_TABLE_H
#define STRING_HASH_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t bucket;
    const char *string;
} string_table_node_t;

typedef struct {
    string_table_node_t **table;
    uint64_t size;
} string_hash_table_t;

string_hash_table_t *string_hash_table_create(uint64_t size);

void string_hash_table_insert(string_hash_table_t *table, const char *string);

bool string_hash_table_contains(string_hash_table_t *table, const char *string);

void string_hash_table_free(string_hash_table_t *table);

#endif

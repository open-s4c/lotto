/*
 */
#include <string.h>

#include <lotto/base/string.h>
#include <lotto/base/string_hash_table.h>
#include <lotto/sys/stdlib.h>

string_hash_table_t *
string_hash_table_create(uint64_t size)
{
    string_hash_table_t *result = sys_malloc(sizeof(string_hash_table_t));
    result->table = sys_calloc(size, sizeof(string_table_node_t *));
    result->size  = size;
    return result;
}

void
string_hash_table_insert(string_hash_table_t *table, const char *string)
{
    uint64_t bucket = string_hash(string) % table->size;
    uint64_t index  = bucket;

    while (table->table[index]) {
        index = (index + 1) % table->size;
    }
    string_table_node_t *node = sys_malloc(sizeof(string_table_node_t));
    node->string              = string;
    node->bucket              = bucket;
    table->table[index]       = node;
}

bool
string_hash_table_contains(string_hash_table_t *table, const char *string)
{
    if (!table->size) {
        return false;
    }
    uint64_t bucket         = string_hash(string) % table->size;
    uint64_t index          = bucket;
    uint64_t starting_index = index;
    bool first_iteration    = true;
    while (table->table[index] && table->table[index]->bucket <= bucket &&
           strcmp(table->table[index]->string, string) != 0 &&
           (starting_index != index || first_iteration)) {
        first_iteration = false;
        index           = (index + 1) % table->size;
    }
    return table->table[index] && table->table[index]->string &&
           strcmp(table->table[index]->string, string) == 0;
}

void
string_hash_table_free(string_hash_table_t *table)
{
    for (uint64_t i = 0; i < table->size; i++) {
        if (!table->table[i]) {
            continue;
        }
        sys_free(table->table[i]);
    }
    sys_free(table->table);
    sys_free(table);
}

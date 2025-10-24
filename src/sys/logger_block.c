/*
 */
#include <limits.h>
#include <string.h>

#include <lotto/base/string.h>
#include <lotto/base/string_hash_table.h>
#include <lotto/sys/logger_block.h>

#define INITIAL_BLOCK_TABLE_LENGTH 256

static string_hash_table_t *blocks;

void
logger_block_init(char *cats)
{
    blocks = string_hash_table_create(INITIAL_BLOCK_TABLE_LENGTH);
    strupr(cats);
    char *token;
    token = strtok(cats, ":|");

    for (; token != NULL; token = strtok(NULL, ":|")) {
        string_hash_table_insert(blocks, token);
    }
}

bool
logger_blocked(const char *file_name)
{
    if (!blocks) {
        return false;
    }
    char catd[PATH_MAX] = {""};
    size_t len          = strcspn(file_name, ".");
    strncpy(catd, file_name, len);
    strupr(catd);
    return string_hash_table_contains(blocks, catd);
}

void
logger_block_fini(void)
{
    if (!blocks) {
        return;
    }
    // string_hash_table_free(blocks);
    blocks = NULL;
}

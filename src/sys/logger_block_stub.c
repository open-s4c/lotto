/*
 */
#include <limits.h>
#include <string.h>

#include <lotto/base/string.h>
#include <lotto/base/string_hash_table.h>
#include <lotto/sys/logger_block.h>

void
logger_block_init(char *cats)
{
}

bool
logger_blocked(const char *file_name)
{
    return false;
}

void
logger_block_fini(void)
{
}

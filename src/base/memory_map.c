#include <ctype.h>
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__)
#    include <malloc/malloc.h>
#endif

#include <lotto/base/memory_map.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/sys/unistd.h>

#define MEMORY_MAP_INIT_LENGTH      1024
#define MEMORY_MAP_INIT_LINE_LENGTH 1024

static memory_map_entry_t *memory_map;

/**
 * /proc/pid/maps format:
 * <address start>-<address end> <mode> <offset> <major id:minor id> <inode id>
 * <file path> where addresses and offsets are in the hexadecimal system.
 * File path is optional and the entty is skipped in case of its absence.
 */
static memory_map_entry_t *
_memory_map_parse(const char *path)
{
    char filename[PATH_MAX];
    const char *maps_filename;
    if (path == NULL) {
        sys_sprintf(filename, "/proc/%d/maps", sys_getpid());
        maps_filename = filename;
    } else {
        maps_filename = path;
    }

    FILE *maps_fp = sys_fopen(maps_filename, "r");
    ASSERT(maps_fp);
    char *line             = NULL;
    size_t memory_map_size = MEMORY_MAP_INIT_LENGTH;
    memory_map_entry_t *result =
        sys_malloc(memory_map_size * sizeof(memory_map_entry_t));
    size_t len = 0;
    size_t i;
    for (i = 0; sys_getline(&line, &len, maps_fp) != -1; i++) {
        char *next;
        result[i].address_start = strtol(line, &next, 16);
        result[i].address_end   = strtol(next + 1, &next, 16); // skip hyphen
        result[i].offset        = strtol(next + 6, &next, 16); // skip mode
        for (next++; *next != '\n' && !isspace(*next);
             next++) // skip device ids
            ;
        for (next++; *next != '\n' && !isspace(*next); next++) // skip inode id
            ;
        for (; *next != '\n' && isspace(*next);
             next++) // skip spacing before the name
            ;
        for (; *next != '\n' && isspace(*next);
             next++) // skip spacing before the name
            ;
        if (*next == '\n') {
            result[i].name[0] = '\0';
        } else {
            // remove \n from length
            size_t name_len = sys_strlen(next) - 1;

            ASSERT(name_len < PATH_MAX);
            sys_memcpy(result[i].name, next, name_len);
            result[i].name[name_len] = '\0';
        }
        if (i == memory_map_size - 1) {
            memory_map_size *= 2;
            result = sys_realloc(result,
                                 memory_map_size * sizeof(memory_map_entry_t));
        }
    }
    result[i] = (memory_map_entry_t){};
    return result;
}

memory_map_entry_t *
memory_map_get(const char *path)
{
    return memory_map ? memory_map : (memory_map = _memory_map_parse(path));
}

void
memory_map_address_lookup_from_path(uintptr_t address, map_address_t *result,
                                    const char *path)
{
#if defined(__APPLE__)
    (void)path;
    ASSERT(result);
    *result = (map_address_t){0};

    Dl_info info;
    if (dladdr((void *)address, &info) != 0 && info.dli_fname != NULL &&
        info.dli_fbase != NULL) {
        result->offset = address - (uintptr_t)info.dli_fbase;
        sys_strcpy(result->name, info.dli_fname);
        return;
    }

    if (malloc_size((void *)address) > 0) {
        result->offset = 0;
        sys_strcpy(result->name, "[heap]");
        return;
    }

    uintptr_t stack_top = (uintptr_t)pthread_get_stackaddr_np(pthread_self());
    size_t stack_size   = pthread_get_stacksize_np(pthread_self());
    uintptr_t stack_bot = stack_top - stack_size;
    if (stack_bot <= address && address < stack_top) {
        result->offset = address - stack_bot;
        sys_strcpy(result->name, "[stack]");
    }
    return;
#else
    memory_map_entry_t *map;
    for (map = memory_map_get(path);
         map->address_start &&
         (map->address_start > address || address >= map->address_end);
         map++)
        ;
    if (!map->address_start) {
        return;
    }
    result->offset = address - map->address_start + map->offset;
    sys_strcpy(result->name, map->name);
#endif
}


void
memory_map_address_lookup(uintptr_t address, map_address_t *result)
{
    memory_map_address_lookup_from_path(address, result, NULL);
}

bool
memory_map_address_equals(const map_address_t *address1,
                          const map_address_t *address2)
{
    ASSERT(address1);
    ASSERT(address2);
    return address1 == address2 ||
           (address1->offset == address2->offset &&
            sys_strcmp(address1->name, address2->name) == 0);
}

int
memory_map_address_sprint(const map_address_t *address, char *output)
{
    ASSERT(address);
    return sys_sprintf(output, "%s:0x%016llx", address->name, address->offset);
}

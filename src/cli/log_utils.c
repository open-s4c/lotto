#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/cli/log_utils.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static int _log_parse(char *ptr, const char *end, log_t *data);

log_t *
log_load(const char *filename, int *count)
{
    FILE *fptr = sys_fopen(filename, "r");
    if (fptr == NULL)
        ASSERT(false && "Cannot read log file");

    sys_fseek(fptr, 0, SEEK_END);
    const long size = sys_ftell(fptr);
    char *raw       = sys_malloc(size + 1);
    const char *end = raw + size;

    sys_rewind(fptr);
    sys_fread(raw, size, 1, fptr);
    raw[size] = '\0';
    sys_fclose(fptr);

    int ctn = _log_parse(raw, end, NULL) + 1;
    ASSERT(ctn > 1 && "Log file is empty");
    log_t *data = sys_malloc(sizeof(log_t) * ctn);
    _log_parse(raw, end, data);

    log_init(data + ctn - 1);
    sys_free(raw);
    if (count != NULL)
        *count = ctn;
    return data;
}

static int
_log_parse(char *ptr, const char *end, log_t *data)
{
    const size_t l_tid  = sys_strlen("tid=");
    const size_t l_id   = sys_strlen("id=");
    const size_t l_data = sys_strlen("data=");

    for (; ptr < end && isspace(*ptr); ptr++) {}
    int i = 0;

    while (ptr < end) {
        ASSERT(!sys_strncmp(ptr, "tid=", l_tid));
        ptr += l_tid;
        if (data != NULL)
            data[i].tid = strtoul(ptr, NULL, 10);
        for (; ptr < end && isdigit(*ptr); ptr++) {}
        for (; ptr < end && isspace(*ptr); ptr++) {}

        ASSERT(!sys_strncmp(ptr, "id=", l_id));
        ptr += l_id;
        if (data != NULL)
            data[i].id = strtoul(ptr, NULL, 10);
        for (; ptr < end && isdigit(*ptr); ptr++) {}
        for (; ptr < end && isspace(*ptr); ptr++) {}

        if (!sys_strncmp(ptr, "data=", l_data)) {
            ptr += l_data;
            if (data != NULL)
                data[i].data = strtoul(ptr, NULL, 16);
            for (; ptr < end && isalnum(*ptr); ptr++) {}
            for (; ptr < end && isspace(*ptr); ptr++) {}
        } else if (data != NULL)
            data[i].data = 0;
        i++;
    }
    return i;
}

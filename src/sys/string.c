/*
 */

#include <string.h>

#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

char *
sys_strdup(const char *s)
{
    if (!s)
        return NULL;
    size_t len = sys_strlen(s);
    char *buf  = sys_malloc(len + 1);
    if (!buf)
        return NULL;
    sys_memcpy(buf, s, len);
    buf[len] = '\0';
    return buf;
}

char *
sys_strndup(const char *s, size_t n)
{
    if (!s)
        return NULL;
    size_t slen = sys_strlen(s);
    size_t len  = slen < n ? slen : n;
    char *buf   = sys_malloc(len + 1);
    if (!buf)
        return NULL;
    sys_memcpy(buf, s, len);
    buf[len] = '\0';
    return buf;
}

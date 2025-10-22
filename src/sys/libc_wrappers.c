/*
 * Minimal libc wrappers required by the trimmed lotto runtime build.
 * These functions mirror the signatures generated in lotto's sys headers
 * but forward directly to the system implementations.
 */
#include <stdlib.h>
#include <string.h>

#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

long
sys_strtol(const char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}

long long
sys_strtoll(const char *nptr, char **endptr, int base)
{
    return strtoll(nptr, endptr, base);
}

void *
sys_malloc(size_t n)
{
    return malloc(n);
}

void *
sys_calloc(size_t n, size_t s)
{
    return calloc(n, s);
}

void *
sys_realloc(void *p, size_t n)
{
    return realloc(p, n);
}

void
sys_free(void *p)
{
    free(p);
}

void
sys_exit(int status)
{
    exit(status);
}

int
sys_system(const char *command)
{
    return system(command);
}

int
sys_setenv(const char *name, const char *value, int overwrite)
{
    return setenv(name, value, overwrite);
}

int
sys_unsetenv(const char *name)
{
    return unsetenv(name);
}

char *
sys_getenv(const char *name)
{
    return getenv(name);
}

void *
sys_memcpy(void *dst, const void *src, size_t n)
{
    return memcpy(dst, src, n);
}

void *
sys_memset(void *dst, int c, size_t n)
{
    return memset(dst, c, n);
}

void *
sys_memmove(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

int
sys_memcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

char *
sys_strdup(const char *str)
{
    return strdup(str);
}

char *
sys_strndup(const char *str, size_t len)
{
    size_t n = strnlen(str, len);
    char *copy = malloc(n + 1);
    if (!copy)
        return NULL;
    memcpy(copy, str, n);
    copy[n] = '\0';
    return copy;
}

char *
sys_stpcpy(char *dst, const char *src)
{
#if defined(_GNU_SOURCE) || defined(__APPLE__)
    return stpcpy(dst, src);
#else
    char *d = dst;
    while ((*d++ = *src++) != '\0')
        ;
    return d - 1;
#endif
}

char *
sys_strcpy(char *dst, const char *src)
{
    return strcpy(dst, src);
}

char *
sys_strcat(char *dst, const char *src)
{
    return strcat(dst, src);
}

int
sys_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int
sys_strncmp(const char *s1, const char *s2, size_t n)
{
    return strncmp(s1, s2, n);
}

size_t
sys_strlen(const char *s)
{
    return strlen(s);
}

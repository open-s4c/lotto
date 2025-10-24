/*
 */
#ifndef LOTTO_ENGINE_ENVVAR_H
#define LOTTO_ENGINE_ENVVAR_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *name;
    struct {
        const char *sval;
        uint64_t uval;
    };
} envvar_t;
void envvar_set(envvar_t *vars, bool overwrite);

#endif

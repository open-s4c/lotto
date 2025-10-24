/*
 */
#ifndef LOTTO_CLI_SUBCMD_H
#define LOTTO_CLI_SUBCMD_H

#include <lotto/cli/args.h>
#include <lotto/cli/flagmgr.h>

typedef int(subcmd_f)(args_t *args, flags_t *flags);
typedef flags_t *(defaults_f)();

enum subcmd_group { SUBCMD_GROUP_RUN, SUBCMD_GROUP_TRACE, SUBCMD_GROUP_OTHER };

typedef struct {
    subcmd_f *func;
    const char *name;
    const char *args;
    const char *desc;
    bool runtime_sel;
    flag_t sel[MAX_FLAGS];
    defaults_f *defaults;
    enum subcmd_group group;
} subcmd_t;

void subcmd_register(subcmd_f *func, const char *name, const char *args,
                     const char *desc, bool runtime_sel, const flag_t sel[],
                     defaults_f *defaults, enum subcmd_group group);
subcmd_t *subcmd_find(const char *name);
subcmd_t *subcmd_find_closest(const char *name);
subcmd_t *subcmd_next(subcmd_t *cur);

#endif

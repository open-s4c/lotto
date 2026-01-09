/*
 */
#include <string.h>

#include <lotto/cli/flagmgr.h>
#include <lotto/cli/subcmd.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/string.h>
#include <lotto/util/strings.h>

#define MAX_SUBCMDS 128

static int _next_reg;
static subcmd_t _subcmd[MAX_SUBCMDS + 1];

void
subcmd_register(subcmd_f *func, const char *name, const char *args,
                const char *desc, bool runtime_sel, const flag_t sel[],
                defaults_f *defaults, enum subcmd_group group)
{
    ASSERT(_next_reg < MAX_SUBCMDS);
    ASSERT(defaults != NULL);
    subcmd_t *subcmd = _subcmd + _next_reg++;
    *subcmd          = (subcmd_t){.func        = func,
                                  .name        = name,
                                  .args        = args,
                                  .desc        = desc,
                                  .runtime_sel = runtime_sel,
                                  .defaults    = defaults,
                                  .group       = group};
    size_t i         = 0;
    for (; i < MAX_FLAGS && sel[i] != 0; i++) {
        subcmd->sel[i] = sel[i];
    }
    for (; i < MAX_FLAGS; i++) {
        subcmd->sel[i] = FLAG_SEL_SENTINEL;
    }
}

subcmd_t *
subcmd_find(const char *name)
{
    /* If is MAX_SUBCMDS+1, then name == NULL. */
    for (int i = 0; _subcmd[i].name; i++) {
        if ((_subcmd[i].name[0] == '-' && name[0] == _subcmd[i].name[0]) ||
            strcmp(_subcmd[i].name, name) == 0) {
            return _subcmd + i;
        }
    }
    return NULL;
}

subcmd_t *
subcmd_find_closest(const char *name)
{
    subcmd_t *closest_subcommand = NULL;
    if (sys_strlen(name) >= 3) {
        uint32_t cur_dist = UINT32_MAX;
        for (int i = 0; _subcmd[i].name; i++) {
            uint32_t dist = levenshtein(_subcmd[i].name, name);
            if (cur_dist > dist) {
                cur_dist           = dist;
                closest_subcommand = _subcmd + i;
            }
        }
    }
    return closest_subcommand;
}

subcmd_t *
subcmd_next(subcmd_t *cur)
{
    if (cur == NULL)
        return _subcmd;
    cur++;
    return cur->name ? cur : NULL;
}

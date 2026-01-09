/*
 */
#include <stdlib.h>

#include <lotto/base/envvar.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/stdio.h>

void
envvar_set(envvar_t *vars, bool overwrite)
{
    for (; vars->name; vars++) {
        if (vars->sval) {
            ASSERT(vars->uval == 0);
            setenv(vars->name, vars->sval, overwrite);
        } else {
            char tmp[1024];
            sys_snprintf(tmp, 1024, "%lu", vars->uval);
            setenv(vars->name, tmp, overwrite);
        }
    }
}

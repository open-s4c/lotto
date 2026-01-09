/*
 */
#include <lotto/cli/args.h>
#include <lotto/sys/stdio.h>

void
args_print(const args_t *args)
{
    for (int i = 0; i < args->argc; i++)
        sys_fprintf(stdout, "%s ", args->argv[i]);
}

void
args_shift(args_t *args, int n)
{
    n = n > args->argc ? args->argc : n;
    args->argc -= n;
    args->argv += n;
}

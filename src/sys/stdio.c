/*
 */

#include <stdio.h>

#include <lotto/sys/ensure.h>
#include <lotto/sys/stdio.h>

void
sys_rewind(FILE *stream)
{
    ENSURE(sys_fseek(stream, 0, SEEK_SET) == 0);
}

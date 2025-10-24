#include <lotto/sys/stdio.h>

int
main()
{
#define smth "a simple test that shouldn't fail\n"
    sys_printf(smth);

    char msg[128];
    sys_sprintf(msg, "hello world: %s", smth);
    printf("%s\n", msg);
    return 0;
}

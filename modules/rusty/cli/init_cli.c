#include <lotto/util/macros.h>

void lotto_rust_cli_init();

static void LOTTO_CONSTRUCTOR
init()
{
    lotto_rust_cli_init();
}

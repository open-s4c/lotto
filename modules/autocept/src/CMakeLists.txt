set(BARE_SOURCES autocept.c subscribers.c module.c)
add_runtime_module(${BARE_SOURCES} default.S)
add_library(autocept-bare.o OBJECT ${BARE_SOURCES})
target_compile_definitions(
    autocept-bare.o
    PRIVATE DICE_MULTIFILE_MODULE LOTTO_RUNTIME_MODULE=1 LOTTO_DRIVER_MODULE=0
            DICE_MODULE_SLOT=46 LOGGER_PREFIX="autocept")
add_driver_module(module.c)

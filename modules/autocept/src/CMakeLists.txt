set(BARE_SOURCES autocept.c subscribers.c module.c)
add_runtime_module(${BARE_SOURCES} default.S)
# add_driver_module(state.c flags.c module.c)
add_library(autocept-bare.o OBJECT ${BARE_SOURCES})

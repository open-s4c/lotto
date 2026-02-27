add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE DICE_MODULE_SLOT=5)

set(SRCS catmgr.c statemgr.c module.c)
add_library(brokers.o OBJECT ${SRCS})
add_library(brokers_testing.o OBJECT ${SRCS})
target_compile_definitions(brokers_testing.o PRIVATE -DLOTTO_TEST)

make_units(brokers "")

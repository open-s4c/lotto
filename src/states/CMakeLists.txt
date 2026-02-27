sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE DICE_MODULE_SLOT=6)

file(GLOB SRCS *.c)
add_library(states.o OBJECT ${SRCS})
add_library(states_testing.o OBJECT ${SRCS})
target_compile_definitions(states_testing.o PRIVATE -DLOTTO_TEST)

make_units(states "")

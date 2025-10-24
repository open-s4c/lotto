sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

add_subdirectory(handlers)

file(GLOB HANDLERS handlers/*.c)
file(GLOB SRCS *.c)
add_library(states.o OBJECT ${SRCS} ${HANDLERS})

add_library(states_testing.o OBJECT ${SRCS})
target_compile_definitions(states_testing.o PRIVATE -DLOTTO_TEST)

make_units(states "")

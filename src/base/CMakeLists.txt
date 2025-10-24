sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)
file(GLOB SRCS *.c)

add_library(base.o OBJECT ${SRCS})

add_library(base_testing.o OBJECT ${SRCS})
target_compile_definitions(base_testing.o PRIVATE -DLOTTO_TEST)

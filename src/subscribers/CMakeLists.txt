include_directories(${CMAKE_BINARY_DIR}/generated)
add_compile_options(-fPIC -Wno-uninitialized)
add_compile_definitions(_GNU_SOURCE)

set(SRCS pthread_create_join.c)
add_library(subscribers.o OBJECT ${SRCS})

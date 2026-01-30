gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)
add_compile_options(-fPIC -Wno-uninitialized)
add_compile_definitions(_GNU_SOURCE)

set(SRCS intercept_pthread_create.c)
add_library(interceptors.o OBJECT ${SRCS})

sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

set(SRCS engine.c)

add_library(flotto_engine.o OBJECT ${SRCS})
target_compile_options(flotto_engine.o PUBLIC -g)

add_library(flotto SHARED)
target_link_libraries(flotto PRIVATE sys.o base.o flotto_engine.o brokers.o)
set_target_properties(flotto PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                        ${CMAKE_BINARY_DIR})

set(LIBTARGET "libflotto.so")
install(TARGETS engine DESTINATION lib)
make_blob(blob-flotto ${LIBTARGET} flotto)

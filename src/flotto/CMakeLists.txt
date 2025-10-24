sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)

set(SRCS engine.c)

add_library(flotto_engine.o OBJECT ${SRCS})
target_compile_options(flotto_engine.o PUBLIC -g)

add_library(flotto SHARED)
target_link_libraries(flotto PRIVATE sys.o base.o flotto_engine.o brokers.o)

set(LIBTARGET "libflotto.so")
install(TARGETS engine DESTINATION lib)
make_blob(blob-flotto ${LIBTARGET} "")

add_custom_command(
    POST_BUILD
    OUTPUT ${CMAKE_BINARY_DIR}/libflotto.so
    COMMAND ${CMAKE_COMMAND} -E copy ${LIBTARGET}
            ${CMAKE_BINARY_DIR}/${LIBTARGET}
    DEPENDS flotto)

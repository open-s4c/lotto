sanitize()
add_compile_definitions(_GNU_SOURCE)
file(GLOB SRCS *.c)
add_library(crep.o OBJECT ${SRCS})
target_link_libraries(crep.o PUBLIC pthread)
target_compile_definitions(crep.o PRIVATE -DCREP_RECORD_ENABLED=1)

add_library(crep SHARED ${SRCS} warn_list.S)
target_link_libraries(crep PRIVATE base.o sys.o)
target_compile_definitions(crep PRIVATE -DCREP_STANDALONE
                                        -DCREP_RECORD_ENABLED=1)

set(TARGET libcrep.so)
add_custom_target(
    libcrep ALL
    BYPRODUCTS ${CMAKE_BINARY_DIR}/libcrep.so
    COMMAND ${CMAKE_COMMAND} -E copy libcrep.so ${CMAKE_BINARY_DIR}/libcrep.so
    DEPENDS crep)

make_blob(blob-crep libcrep.so crep)

add_library(crep_no_record.o OBJECT ${SRCS})
target_link_libraries(crep_no_record.o PUBLIC pthread)
target_compile_definitions(crep_no_record.o PRIVATE -DCREP_RECORD_ENABLED=0)

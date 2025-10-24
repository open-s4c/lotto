file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TARGET ${SRC} NAME_WLE)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64 AND ${TARGET} STREQUAL
                                                      memory_map_test)
        continue()
    endif()
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET} base.o util sys.o memmgr_runtime.o
                          memmgr_user.o)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

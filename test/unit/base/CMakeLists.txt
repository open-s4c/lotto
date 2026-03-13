if(NOT ${LOTTO_RACKET_TESTS})
    return()
endif()

add_subdirectory(mock)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mock)

file(GLOB SRCS *_test.c)
list(REMOVE_ITEM SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tidbag_test.c
     ${CMAKE_CURRENT_SOURCE_DIR}/tidset_test.c SRCS)
foreach(SRC ${SRCS})
    get_filename_component(TARGET ${SRC} NAME_WLE)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET} base_testing.o sys_testing.o
                          memmgr_runtime_libc.o memmgr_user_libc.o)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

file(GLOB SUBSRCS tidbag_test.c tidset_test.c)
foreach(SUBSRC ${SUBSRCS})
    get_filename_component(TARGET ${SUBSRC} NAME_WLE)
    add_executable(${TARGET} ${SUBSRC})
    target_link_libraries(${TARGET} base_testing.o sys_testing.o memock.o dice.o)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

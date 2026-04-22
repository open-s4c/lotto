if(${LOTTO_MOCKOTO_TESTS})
    add_subdirectory(mock)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mock)
endif()

file(GLOB SRCS *_test.c)
list(
    REMOVE_ITEM
    SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/tidbag_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tidset_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/address_bdd_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/casts_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/memory_map_test.c
    SRCS)
foreach(SRC ${SRCS})
    get_filename_component(TARGET ${SRC} NAME_WLE)
    add_executable(${TARGET} ${SRC})
    target_link_libraries(${TARGET} base_testing.o sys_testing.o
                          memmgr_runtime_libc.o memmgr_user_libc.o)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

if(TARGET memock.o)
    file(GLOB SUBSRCS tidbag_test.c tidset_test.c)
    foreach(SUBSRC ${SUBSRCS})
        get_filename_component(TARGET ${SUBSRC} NAME_WLE)
        add_executable(${TARGET} ${SUBSRC})
        target_link_libraries(${TARGET} base_testing.o sys_testing.o memock.o
                              dice.o)
        add_test(NAME ${TARGET} COMMAND ${TARGET})
    endforeach()
endif()

file(GLOB UTILSRCS address_bdd_test.c casts_test.c memory_map_test.c)
foreach(UTILSRC ${UTILSRCS})
    get_filename_component(TARGET ${UTILSRC} NAME_WLE)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64 AND ${TARGET} STREQUAL
                                                      memory_map_test)
        continue()
    endif()
    add_executable(${TARGET} ${UTILSRC})
    target_link_libraries(${TARGET} base.o util sys.o memmgr_runtime_libc.o
                          memmgr_user_libc.o)
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endforeach()

if(${LOTTO_RACKET_TESTS} AND TARGET base_component)
    add_racket_test(
        FILENAME plan_test.rkt
        BINDING lotto/engine/plan.h
        DEPENDENCIES base_component)

    add_racket_test(
        FILENAME tidset_test.rkt
        BINDING lotto/engine/tidset.h
        DEPENDENCIES base_component)

    add_racket_test(FILENAME bag_test.rkt)

    add_racket_test(FILENAME tidmap_test.rkt)

    add_racket_test(FILENAME tidbag_test.rkt)

    add_racket_test(FILENAME trace_flat_test.rkt)
endif()

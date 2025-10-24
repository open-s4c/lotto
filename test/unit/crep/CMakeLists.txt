# add_mock( TARGET .c INCLUDE lotto/sys/memory.h EXCLUDE io.h logger.h FLAGS -I
# ${PROJECT_SOURCE_DIR}/deps/target/include -I
# /usr/lib/gcc/x86_64-linux-gnu/11/include) file(GLOB SRCS stream*.c)
# add_library(memock.o OBJECT memory_mock.c) target_link_libraries(memock.o
# PRIVATE dl) target_compile_definitions(memock.o PRIVATE -DLOTTO_TEST)
#

set(LIT_CREP_SRCS "")
set(LIT_CREP_DEPS "")
file(GLOB SRCS *_test.c)
foreach(SRC ${SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_executable(${TEST} ${SRC})
    target_link_libraries(${TEST} PUBLIC crep pthread memmgr_runtime.o
                                         memmgr_user.o)
    set(COPIED_SRC "${CMAKE_CURRENT_BINARY_DIR}/lit/${TEST}.c")
    add_lit_target(lit-${TEST} ${LIT_WORKERS} ${LIT_TIMEOUT} COPIED_SRC)
    list(APPEND LIT_CREP_SRCS ${COPIED_SRC})
    list(APPEND LIT_CREP_DEPS ${TEST})
    add_dependencies(lit-${TEST} lit-prepare ${TEST})
endforeach()

# ##############################################################################
# Lit setup
# ##############################################################################

set(LIT_CFG lit/lit.cfg)
configure_file(lit.cfg.in ${LIT_CFG} @ONLY)
set(LIT_CFG_PATH ${CMAKE_CURRENT_BINARY_DIR}/${LIT_CFG})
add_lit_prepare()
add_lit_target(lit-crep ${LIT_WORKERS} ${LIT_DAILY_TIMEOUT} LIT_CREP_SRCS)
add_dependencies(lit-crep lit-prepare ${LIT_CREP_DEPS})

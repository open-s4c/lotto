sanitize()
add_compile_options(-fPIC)
add_compile_definitions(_GNU_SOURCE)
set(SRCS
    abort.c
    common.c
    stdio.c
    logger.c
    memory.c
    mempool.c
    string.c
    plugin.c
    stream_chunked_file.c
    stream_chunked_impl.c
    stream_ffile.c
    stream_file.c
    stream_impl.c)

if("${LOTTO_LOGGER_BLOCK}")
    list(APPEND SRCS logger_block.c)
else()
    list(APPEND SRCS logger_block_stub.c)
endif()

add_library(abort.o OBJECT abort.c)

add_library(memmgr_runtime.o OBJECT memmgr.c)
target_compile_definitions(memmgr_runtime.o PRIVATE LOTTO_MEMMGR_RUNTIME)

add_library(memmgr_user.o OBJECT memmgr.c)
target_compile_definitions(memmgr_user.o PRIVATE LOTTO_MEMMGR_USER)

add_library(sys.o OBJECT ${SRCS})
target_link_libraries(sys.o PRIVATE dl memmgr_runtime.o memmgr_user.o)

add_library(sys SHARED ${SRCS})
target_link_libraries(sys PRIVATE dl memmgr_runtime.o memmgr_user.o)

foreach(SRC ${SRCS})
    get_filename_component(BASE ${SRC} NAME_WLE)
    set(TARGET sys_${BASE}.o)
    add_library(${TARGET} OBJECT ${SRC})
endforeach()

file(GLOB EXCLUDES memmgr.c)
make_units(sys ${EXCLUDES})

add_library(sys_testing.o OBJECT ${SRCS})
target_link_libraries(sys_testing.o PRIVATE dl)
target_compile_definitions(sys_testing.o PRIVATE -DLOTTO_TEST)

list(REMOVE_ITEM SRCS real_func.c)
add_library(sys_unreal_testing.o OBJECT ${SRCS})
target_link_libraries(sys_unreal_testing.o PRIVATE dl)
target_compile_definitions(sys_unreal_testing.o PRIVATE -DLOTTO_TEST)

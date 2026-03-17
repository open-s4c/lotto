sanitize()
add_compile_definitions(_GNU_SOURCE DICE_MODULE_SLOT=9)
add_subdirectory(flags)

include_directories(${CMAKE_BINARY_DIR}/generated)

set(CLI_UTILS_SRCS
	module.c
    utils.c
    files.c
    subcmd.c
    args.c
    flagmgr.c
    exec.c
    exec_info.c
    trace_utils.c
    preload.c)

file(GLOB OPTIONAL_SRCS log_utils.c)
list(APPEND CLI_UTILS_SRCS ${OPTIONAL_SRCS})

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
    set_source_files_properties(preload.c PROPERTIES SKIP_LINTING ON)
endif()

macro(add_driver_dependencies TARGET)
    add_dependencies(
        "${TARGET}"
        blob-tsano
        blob-lotto
        blob-lotto-verbose
        ${ARGN})
    target_include_directories(${TARGET} PUBLIC ${CMAKE_BINARY_DIR}/include)
endmacro()

add_library(cli.o OBJECT ${CLI_UTILS_SRCS})
target_compile_options(cli.o PRIVATE -g)
target_link_libraries(cli.o PUBLIC m)
add_driver_dependencies(cli.o)

add_library(cli_testing.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_testing.o PRIVATE -DLOTTO_TEST)
target_compile_options(cli_testing.o PRIVATE -g)
target_link_libraries(cli_testing.o PUBLIC m)
add_driver_dependencies(cli_testing.o)

file(GLOB DRIVER_SRCS "commands/*.c")

foreach(HANDLER ${LOTTO_DISABLE_HANDLERS})
    list(REMOVE_ITEM DRIVER_SRCS
         "${CMAKE_CURRENT_SOURCE_DIR}/commands/cli_${HANDLER}.c")
endforeach()

file(GLOB_RECURSE FLAG_SRCS "flags/*.c")

add_library(lotto-driver SHARED main.c ${FLAG_SRCS} ${DRIVER_SRCS})
target_link_libraries(
    lotto-driver
    PRIVATE cli.o
            base.o
            sys.o
            memmgr_runtime_libc.o
            memmgr_user_libc.o
            lotto-driver-engine.o
            dice.o)
target_compile_options(lotto-driver PRIVATE -g)
target_compile_definitions(lotto-driver
                           PRIVATE LOTTO_VERSION="${PROJECT_VERSION}")
set_target_properties(lotto-driver PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                              ${CMAKE_BINARY_DIR})

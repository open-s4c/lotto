sanitize()
add_compile_definitions(_GNU_SOURCE)
add_subdirectory(gdb)
add_subdirectory(flags)
add_subdirectory(python)
# ##############################################################################
# CLI static library of utils
# ##############################################################################

gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)

set(CLI_UTILS_SRCS
    utils.c
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
    # clang-tidy wastes a lot of time on the huge included blobs.
    set_source_files_properties(preload.c PROPERTIES SKIP_LINTING ON)
endif()

macro(add_cli_dependencies TARGET)
    add_dependencies(
        "${TARGET}"
        blob-tsano
        blob-util
        blob-debug
        blob-command
        blob-decorator
        blob-filter
        blob-iterator
        blob-matcher
        blob-handler
        blob-lotto
        blob-lotto-verbose
        ${ARGN})
    target_include_directories(${TARGET} PUBLIC ${CMAKE_BINARY_DIR}/include)
endmacro()

add_library(cli.o OBJECT ${CLI_UTILS_SRCS})
target_compile_options(cli.o PRIVATE -g)
target_link_libraries(cli.o PUBLIC m)
add_cli_dependencies(cli.o)

add_library(cli_testing.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_testing.o PRIVATE -DLOTTO_TEST)
target_compile_options(cli_testing.o PRIVATE -g)
target_link_libraries(cli_testing.o PUBLIC m)
add_cli_dependencies(cli_testing.o)

# ##############################################################################
# CLI lotto command
# ##############################################################################
file(COPY python DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB CLI_SRCS "commands/*.c")
if(NOT "${LOTTO_DRUM}")
    list(REMOVE_ITEM CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/commands/cli_drum.c)
endif()

foreach(HANDLER ${LOTTO_DISABLE_HANDLERS})
    list(REMOVE_ITEM CLI_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/cli_${HANDLER}.c")
endforeach()

file(GLOB_RECURSE FLAG_SRCS "flags/*.c")

add_library(lotto-driver SHARED ${FLAG_SRCS} ${CLI_SRCS})
target_link_libraries(
    lotto-driver
    PRIVATE cli.o
            states.o
            brokers.o
            base.o
            sys.o
            memmgr_runtime.o
            memmgr_user.o
            dice.o)
target_compile_options(lotto-driver PRIVATE -g)
target_compile_definitions(lotto-driver
                           PRIVATE LOTTO_VERSION="${PROJECT_VERSION}")

if("${LOTTO_DRUM}")
    target_link_libraries(lotto-driver PRIVATE curses)
endif()

add_executable(lotto-cli main.c)
target_compile_options(lotto-cli PRIVATE -g)
target_link_libraries(lotto-cli PRIVATE lotto-driver)
target_include_directories(lotto-cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(lotto-cli PRIVATE LOTTO_REAL_NEXT
                                             LOTTO_VERSION="${PROJECT_VERSION}")
set_target_properties(
    lotto-cli
    PROPERTIES ENABLE_EXPORTS ON
               OUTPUT_NAME lotto
               RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

install(TARGETS lotto-cli DESTINATION bin)

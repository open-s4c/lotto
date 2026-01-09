add_compile_definitions(_GNU_SOURCE)
# add_subdirectory(gdb)
add_subdirectory(flags)
# add_subdirectory(python)
# ##############################################################################
# CLI static library of utils
# ##############################################################################

gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)

set(CLI_UTILS_SRCS
    args.c
    exec.c
    exec_info.c
    flagmgr.c
    preload.c
    subcmd.c
    trace_utils.c
    utils.c
    flags/prng.c
    ../base/flags.c
    ../base/envvar.c
    ../base/reason.c
    ../base/trace_file.c
    ../base/trace_impl.c
    ../base/trace_flat.c
    ../base/trace_chunked.c
    ../sys/common.c
    ../sys/stream_file.c
    ../sys/stream_chunked_file.c
    ../sys/stream_chunked_impl.c
    ../states/handlers/handler_available.c
    #
)

list(APPEND CLI_UTILS_SRCS ${OPTIONAL_SRCS})

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
    # clang-tidy wastes a lot of time on the huge included blobs.
    set_source_files_properties(preload.c PROPERTIES SKIP_LINTING ON)
endif()

add_library(cli_noscheme.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_noscheme.o PRIVATE -D_GNU_SOURCE)
target_compile_options(cli_noscheme.o PRIVATE -g)
target_include_directories(cli_noscheme.o PUBLIC ${CMAKE_BINARY_DIR}/include)
target_include_directories(cli_noscheme.o
                           PUBLIC ${PROJECT_BINARY_DIR}/generated)
macro(add_cli_dependencies TARGET)
    add_dependencies(
        "${TARGET}"
        # blob-plotto blob-plotto-verbose blob-tsano blob-crep blob-util
        # blob-debug blob-command blob-decorator blob-filter blob-iterator
        # blob-matcher blob-handler blob-runtime blob-engine blob-flotto
        ${ARGN})
endmacro()
# add_cli_dependencies(cli_noscheme.o)

add_library(cli.o OBJECT ${CLI_UTILS_SRCS})
target_compile_options(cli.o PRIVATE -g)
target_include_directories(cli.o PUBLIC ${CMAKE_BINARY_DIR}/include)

add_library(cli_testing.o OBJECT ${CLI_UTILS_SRCS})
target_compile_options(cli_testing.o PRIVATE -g)
target_include_directories(cli_testing.o PUBLIC ${CMAKE_BINARY_DIR}/include)

# ##############################################################################
# CLI lotto command
# ##############################################################################

set(CLI_SRCS commands/cli_drum.c commands/cli_stress.c commands/cli_run.c
             commands/cli_lotto.c commands/cli_show.c)

if(NOT "${LOTTO_DRUM}")
    list(REMOVE_ITEM CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/commands/cli_drum.c)
endif()

foreach(HANDLER ${LOTTO_DISABLE_HANDLERS})
    list(REMOVE_ITEM CLI_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/cli_${HANDLER}.c")
endforeach()

add_executable(lotto-cli main.c ${FLAG_SRCS} ${CLI_SRCS})
target_compile_definitions(lotto-cli PRIVATE LOTTO_REAL_NEXT
                                             LOTTO_VERSION="${PROJECT_VERSION}")

target_link_libraries(
    lotto-cli
    PRIVATE # states.o brokers.o base.o sys.o crep_no_record.o
            cli.o
            # memmgr_runtime.o memmgr_user.o
            lotto_runtime_cli dice)
# if("${LOTTO_DRUM}")
target_link_libraries(lotto-cli PRIVATE curses)
# endif()
target_compile_options(lotto-cli PRIVATE -g)
set_target_properties(lotto-cli PROPERTIES ENABLE_EXPORTS ON)
file(COPY python DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(lotto-cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(
    lotto-cli PROPERTIES OUTPUT_NAME lotto RUNTIME_OUTPUT_DIRECTORY
                                           ${CMAKE_BINARY_DIR})
install(TARGETS lotto-cli DESTINATION bin)

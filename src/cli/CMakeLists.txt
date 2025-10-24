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

add_library(cli_noscheme.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_noscheme.o PRIVATE -D_GNU_SOURCE)
target_compile_options(cli_noscheme.o PRIVATE -g)
target_include_directories(cli_noscheme.o PUBLIC ${CMAKE_BINARY_DIR}/include)
target_include_directories(cli_noscheme.o
                           PUBLIC ${PROJECT_BINARY_DIR}/generated)
macro(add_cli_dependencies TARGET)
    add_dependencies(
        "${TARGET}"
        blob-plotto
        blob-plotto-verbose
        blob-tsano
        blob-crep
        blob-util
        blob-debug
        blob-command
        blob-decorator
        blob-filter
        blob-iterator
        blob-matcher
        blob-handler
        blob-runtime
        blob-engine
        blob-flotto
        ${ARGN})
endmacro()
add_cli_dependencies(cli_noscheme.o)

if("${LOTTO_CHIBI}")
    set(CHIBI_FLAGS "-DLOTTO_CHIBI")
    set(CHIBI_LIBS chibi-scheme m)
    set(CHIBI_DEPS blob-init-7 blob-meta-7)
    list(APPEND CLI_UTILS_SRCS scheme.c)
    make_blob(blob-init-7
              ${PROJECT_SOURCE_DIR}/deps/chibi-scheme/lib/init-7.scm
              chibi-scheme)
    make_blob(blob-meta-7
              ${PROJECT_SOURCE_DIR}/deps/chibi-scheme/lib/meta-7.scm
              chibi-scheme)
endif()

add_library(cli.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli.o PRIVATE "${CHIBI_FLAGS}")
target_compile_options(cli.o PRIVATE -g)
target_link_libraries(cli.o PUBLIC ${CHIBI_LIBS})
target_include_directories(cli.o PUBLIC ${CMAKE_BINARY_DIR}/include)
add_cli_dependencies(cli.o ${CHIBI_DEPS})

add_library(cli_testing.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_testing.o PRIVATE -DLOTTO_TEST "${CHIBI_FLAGS}")
target_compile_options(cli_testing.o PRIVATE -g)
target_link_libraries(cli_testing.o PUBLIC ${CHIBI_LIBS})
target_include_directories(cli_testing.o PUBLIC ${CMAKE_BINARY_DIR}/include)
add_cli_dependencies(cli_testing.o ${CHIBI_DEPS})

# ##############################################################################
# CLI lotto command
# ##############################################################################

file(GLOB CLI_SRCS "commands/*.c")

if(NOT "${LOTTO_DRUM}")
    list(REMOVE_ITEM CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/commands/cli_drum.c)
endif()

if(NOT "${LOTTO_CHIBI}")
    list(REMOVE_ITEM CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/commands/scheme.c)
endif()

foreach(HANDLER ${LOTTO_DISABLE_HANDLERS})
    list(REMOVE_ITEM CLI_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/cli_${HANDLER}.c")
endforeach()

file(GLOB_RECURSE FLAG_SRCS "flags/*.c")
add_executable(lotto-cli main.c ${FLAG_SRCS} ${CLI_SRCS})
target_compile_definitions(lotto-cli PRIVATE LOTTO_REAL_NEXT
                                             LOTTO_VERSION="${PROJECT_VERSION}")

target_link_libraries(
    lotto-cli
    PRIVATE states.o
            brokers.o
            base.o
            sys.o
            crep_no_record.o
            cli.o
            memmgr_runtime.o
            memmgr_user.o)
if("${LOTTO_DRUM}")
    target_link_libraries(lotto-cli PRIVATE curses)
endif()
target_compile_options(lotto-cli PRIVATE -g)
set_target_properties(lotto-cli PROPERTIES ENABLE_EXPORTS ON)
file(COPY python DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(lotto-cli PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(
    lotto-cli PROPERTIES OUTPUT_NAME lotto RUNTIME_OUTPUT_DIRECTORY
                                           ${CMAKE_BINARY_DIR})
install(TARGETS lotto-cli DESTINATION bin)

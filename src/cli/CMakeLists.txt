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
    #preload.c
)

file(GLOB OPTIONAL_SRCS log_utils.c)
list(APPEND CLI_UTILS_SRCS ${OPTIONAL_SRCS})

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
    # clang-tidy wastes a lot of time on the huge included blobs.
    set_source_files_properties(preload.c PROPERTIES SKIP_LINTING ON)
endif()

macro(add_cli_dependencies TARGET)
    add_dependencies(
        "${TARGET}"
        blob-plotto.h
        blob-plotto-verbose.h
        blob-tsano.h
        blob-util.h
        blob-debug.h
        blob-command.h
        blob-decorator.h
        blob-filter.h
        blob-iterator.h
        blob-matcher.h
        blob-handler.h
        blob-runtime.h
        blob-engine.h
        ${ARGN})
endmacro()

add_library(preload.o OBJECT preload.c)
add_cli_dependencies(preload.o)

add_library(cli.o OBJECT ${CLI_UTILS_SRCS})
target_compile_options(cli.o PRIVATE -g)
target_include_directories(cli.o PUBLIC ${CMAKE_BINARY_DIR}/include)
target_link_libraries(cli.o PUBLIC m)
target_link_libraries(cli.o PRIVATE preload.o)
#add_cli_dependencies(cli.o)

add_library(cli_testing.o OBJECT ${CLI_UTILS_SRCS})
target_compile_definitions(cli_testing.o PRIVATE -DLOTTO_TEST)
target_compile_options(cli_testing.o PRIVATE -g)
target_include_directories(cli_testing.o PUBLIC ${CMAKE_BINARY_DIR}/include)
target_link_libraries(cli_testing.o PUBLIC m)
target_link_libraries(cli_testing.o PRIVATE preload.o)
#add_cli_dependencies(cli_testing.o)

# ##############################################################################
# CLI lotto command
# ##############################################################################

file(GLOB CLI_SRCS "commands/*.c")

if(NOT "${LOTTO_DRUM}")
    list(REMOVE_ITEM CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/commands/cli_drum.c)
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
            cli.o
            memmgr_runtime.o
            memmgr_user.o
            dice)
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

add_library(driver SHARED ${FLAG_SRCS} ${CLI_SRCS} ${CLI_UTILS_SRCS})
target_link_libraries(driver
        PRIVATE states.o
        brokers.o
        base.o
        sys.o
        crep_no_record.o
        memmgr_runtime.o
        memmgr_user.o
        dice)
target_compile_options(driver PRIVATE -g)
target_compile_definitions(driver PRIVATE LOTTO_VERSION="${PROJECT_VERSION}")
target_link_libraries(driver PRIVATE preload.o)
#add_cli_dependencies(driver)

add_executable(lotto-cli-new main.c)
target_link_libraries(lotto-cli-new PUBLIC driver)
target_link_libraries(lotto-cli-new PRIVATE preload.o)
#add_cli_dependencies(lotto-cli-new)

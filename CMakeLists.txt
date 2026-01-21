cmake_minimum_required(VERSION 3.20)
project(lotto LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(PROJECT_IS_TOP_LEVEL)
    # For now we just globally overwrite the default flags for the release
    # profile to _NOT_ define `-DNDEBUG`, for assertions to work. In the future
    # we might want to make this more fine-grained and e.g. disable assertions
    # in the CLI in release, or partially in the lotto engine.
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
endif()
add_compile_options("-fPIC" "-Werror")

# ##############################################################################
# Include stable options (shared with release)
# ##############################################################################
set(LOTTO_FRONTEND_LIST POSIX QEMU)
include(cmake/stable_options.cmake)

option(LOTTO_LTO "Enable LTO for whole project" OFF)

# ##############################################################################
# Dependencies
# ##############################################################################
set(DICE_LOG_LEVEL
    "INFO"
    CACHE STRING "" FORCE)

set(DICE_LTO
    ${LOTTO_LTO}
    CACHE BOOL "" FORCE)

set(DICE_DISPATCH_CHAINS
    "1,4,7,2,3,5,6,0"
    CACHE STRING "" FORCE)

set(DICE_DISABLE_LOADER
    on
    CACHE BOOL "" FORCE)

add_subdirectory(deps)

find_package(libvsync 4.2 CONFIG REQUIRED)
link_libraries(libvsync::vsync)
add_library(vsync ALIAS libvsync::vsync)

# ------------------------------------------------------------------------------
# CMake libs
# ------------------------------------------------------------------------------
# include(cmake/submodule.cmake)

include(cmake/utils.cmake)

# ##############################################################################
# Generic project configuration
# ##############################################################################
# pkgtools_add_clang_format_targets() pkgtools_add_cmake_format_targets()
# pkgtools_set_version_from_tag()

# ##############################################################################
# Experimental options
# ##############################################################################
if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    set(LOTTO_STABLE_ADDRESS_MAP_DEFAULT OFF)
    set(LOTTO_QEMU_YIELD_DEFAULT OFF)
else()
    set(LOTTO_STABLE_ADDRESS_MAP_DEFAULT ON)
    set(LOTTO_QEMU_YIELD_DEFAULT OFF)
endif()

option(LOTTO_STABLE_ADDRESS_MAP
       "whether stable addresses support the `MAP` method"
       ${LOTTO_STABLE_ADDRESS_MAP_DEFAULT})

if("${LOTTO_STABLE_ADDRESS_MAP}")
    add_compile_definitions(LOTTO_STABLE_ADDRESS_MAP)
    set(LOTTO_STABLE_ADDRESS_MAP_LIT_FEATURE STABLE_ADDRESS_MAP)
endif()

option(LOTTO_QEMU_YIELD "whether to instrument guest code with yield syscall"
       ${LOTTO_QEMU_YIELD_DEFAULT})

include(cmake/pubsub.cmake)

# ##############################################################################
# testing configuration
# ##############################################################################
option(LOTTO_RACKET_TESTS "Enable Racket tests" OFF)
option(LOTTO_TEST_COVERAGE "Enable test coverage with lcov" OFF)

if("${LOTTO_TEST_COVERAGE}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

add_custom_target(coverage COMMAND gcovr -r ${PROJECT_SOURCE_DIR}/src
                                   ${PROJECT_BINARY_DIR} | cut -b -64)

enable_testing()
include(cmake/mockoto.cmake)
include(cmake/racket.cmake)

# ##############################################################################
# source
# ##############################################################################
add_compile_options(
    -Werror
    -Wall
    -Wunused-variable
    -Wextra
    -Wno-missing-braces
    -Wno-switch
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    $<$<COMPILE_LANGUAGE:C>:-Wno-override-init>)

if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND PROJECT_IS_TOP_LEVEL)
    if("${CMAKE_C_COMPILER_VERSION}" VERSION_GREATER_EQUAL 12)
        add_compile_options(-Wunused-but-set-variable)
        add_compile_options(-Wno-cast-function-type)
        add_compile_options(-Wno-compound-token-split-by-macro)
    endif()

    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wshorten-64-to-32>
                        -Wno-initializer-overrides)
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set(CMAKE_C_CLANG_TIDY clang-tidy
                               "-extra-arg=-Wno-unknown-warning-option" "-fix")
    else()
        message(STATUS "clang-tidy not found. Skipping.")
    endif()
else() # gcc
    add_compile_options(-Wunused-but-set-variable)
    add_compile_options(-Wno-cast-function-type)
endif()

set(LOTTO_PLUGIN_INSTALL_DIR "share/lotto/plugins")
set(LOTTO_PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/plugins")

set(QLOTTO_PLUGIN_INSTALL_DIR "share/lotto/qlotto/plugins")
set(QLOTTO_PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/qlotto/plugins")

include_directories(include)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(demos)
add_subdirectory(test)
add_subdirectory(doc)
add_subdirectory(plugins)

add_subdirectory(scripts)

# ##############################################################################
# yapf
# ##############################################################################
macro(yapf MODE IGNORE_FILE ARGS)
    add_custom_target(
        yapf-${MODE}
        COMMAND
            git ls-files ${CMAKE_CURRENT_SOURCE_DIR} #
            | grep -E [['\.(py|pyi|pyw)$$']] #
            | grep -E --invert-match -f ${IGNORE_FILE} #
            | xargs yapf ${ARGS})
endmacro()

yapf(check ${CMAKE_CURRENT_SOURCE_DIR}/.yapf.ignore --diff)
yapf(apply ${CMAKE_CURRENT_SOURCE_DIR}/.yapf.ignore -i)

# ##############################################################################
# Documentation options
# ##############################################################################
include(cmake/doc.cmake)

add_doc_target(api ${CMAKE_CURRENT_SOURCE_DIR}/include main
               ${CMAKE_CURRENT_SOURCE_DIR}/doc/api)
add_doc_target(dev ${CMAKE_CURRENT_SOURCE_DIR}/src/include main
               ${CMAKE_CURRENT_SOURCE_DIR}/doc/dev)

# ##############################################################################
# Custom newline check and apply targets for git controlled header files
# ##############################################################################

add_custom_target(
    newline-format-check
    COMMAND
        ${CMAKE_COMMAND} -E env "SILENT=true"
        ${PROJECT_SOURCE_DIR}/scripts/newline-format.sh ${PROJECT_SOURCE_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_custom_target(
    newline-format-apply
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/newline-format.sh --apply
            ${PROJECT_SOURCE_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

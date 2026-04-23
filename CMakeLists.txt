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
add_compile_options("-fPIC" "-Werror" -DLOG_UNLOCKED)

# ##############################################################################
# Include stable options (shared with release)
# ##############################################################################
set(LOTTO_FRONTEND_LIST POSIX QEMU)
include(cmake/detect.cmake)
include(cmake/stable_options.cmake)
include(cmake/submodule.cmake)

# ##############################################################################
# Dependencies
# ##############################################################################

set(DICE_LOG_LEVEL
    "FATAL"
    CACHE STRING "" FORCE)

set(DICE_MEMSET
    OFF
    CACHE BOOL "" FORCE)

set(DICE_MEMPOOL_USE_MMAP
    ON
    CACHE BOOL "" FORCE)

set(DICE_MEMPOOL_SIZE
    "(1024ULL * 1024ULL * 1024ULL * 2ULL)"
    CACHE STRING "" FORCE)

set(LTO OFF)
if(${CMAKE_INTERPROCEDURAL_OPTIMIZATION})
    set(LTO ON)
endif()

set(DICE_LTO
    ${LOTTO_LTO}
    CACHE BOOL "" FORCE)

set(DICE_MAIN_HOOKS
    OFF
    CACHE BOOL "" FORCE)

set(DICE_DISPATCH_CHAINS
    "1,4,7,2,3,5,6,0"
    CACHE STRING "" FORCE)

set(DICE_MAX_BUILTIN_SLOTS
    60
    CACHE STRING "" FORCE)

set(DICE_MAX_TYPES
    200
    CACHE STRING "" FORCE)

add_compile_definitions(DICE_XTOR_PRIO=1000)
add_compile_definitions(LOG_PREFIX="[ dice] ")
add_compile_definitions(LOG_LEVEL=${DICE_LOG_LEVEL})

add_subdirectory(deps)
link_libraries(dice.h)
set_property(TARGET dice.h PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)

find_package(libvsync 4.2 CONFIG REQUIRED)
link_libraries(libvsync::vsync)
add_library(vsync ALIAS libvsync::vsync)

# Installed Lotto public headers expose libvsync types/macros, so install the
# corresponding headers into the Lotto prefix as well.
get_target_property(LOTTO_VSYNC_INCLUDE_DIRS libvsync::vsync
                    INTERFACE_INCLUDE_DIRECTORIES)
if(LOTTO_VSYNC_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES LOTTO_VSYNC_INCLUDE_DIRS)
    foreach(LOTTO_VSYNC_INCLUDE_DIR ${LOTTO_VSYNC_INCLUDE_DIRS})
        if(EXISTS "${LOTTO_VSYNC_INCLUDE_DIR}/vsync")
            install(DIRECTORY "${LOTTO_VSYNC_INCLUDE_DIR}/vsync"
                    DESTINATION include)
        endif()
    endforeach()
endif()

# ------------------------------------------------------------------------------
# tikl tests options
# ------------------------------------------------------------------------------

message(STATUS "TIKL_PROGRAM: ${TIKL_PROGRAM}")

set(LOTTO_TIKL_TIMEOUT
    300
    CACHE STRING "tikl timeout")

set(LOTTO_TIKL_WORKERS
    1
    CACHE STRING "Number of tikl workers")

# ------------------------------------------------------------------------------
# CMake libs
# ------------------------------------------------------------------------------
include(cmake/utils.cmake)
hide_cache_variables("^DICE_")
hide_cache_variables("^TSANO_")
hide_cache_variables("^CAPSTONE_")
hide_cache_variables("^BUILD_SHARED_LIBS$")
hide_cache_variables("^BUILD_STATIC_LIBS$")
hide_cache_variables("^BUILD_STATIC_RUNTIME$")

set(LOTTO_MODULE_TYPE_LIST PLUGIN BUILTIN)
include(cmake/modules.cmake)

# ##############################################################################
# Generic project configuration
# ##############################################################################
# pkgtools_add_clang_format_targets() pkgtools_add_cmake_format_targets()
# pkgtools_set_version_from_tag()

# ##############################################################################
# Experimental options
# ##############################################################################
set(LOTTO_STABLE_ADDRESS_MAP_LIT_FEATURE STABLE_ADDRESS_MAP)
set(LOTTO_QEMU_YIELD_DEFAULT OFF)

option(LOTTO_QEMU_YIELD "whether to instrument guest code with yield syscall"
       ${LOTTO_QEMU_YIELD_DEFAULT})

# ##############################################################################
# testing configuration
# ##############################################################################
option(LOTTO_RACKET_TESTS "Enable Racket tests" OFF)
option(LOTTO_MOCKOTO_TESTS
       "Enable tests that depend on mockoto-generated mock/binding artifacts"
       OFF)
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
    -Wno-error
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

set(LOTTO_MODULE_INSTALL_DIR "lib/lotto")
set(LOTTO_MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/modules")

set(QLOTTO_MODULE_INSTALL_DIR "lib/lotto/qlotto")
set(QLOTTO_MODULE_BUILD_DIR "${CMAKE_BINARY_DIR}/qlotto/modules")

add_custom_target(tikl-test)
include_directories(include)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(modules)
add_subdirectory(test)
add_subdirectory(demos)
add_subdirectory(doc)

add_subdirectory(scripts)

set(LOTTO_CICD_INSTALL_BUILD_DIR "${CMAKE_BINARY_DIR}/cicd-install-build")
set(LOTTO_CICD_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/cicd-install-prefix")
add_custom_target(
    cicd-install
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${LOTTO_CICD_INSTALL_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${LOTTO_CICD_INSTALL_PREFIX}"
    COMMAND
        ${CMAKE_COMMAND} -S "${CMAKE_SOURCE_DIR}" -B
        "${LOTTO_CICD_INSTALL_BUILD_DIR}" -G "${CMAKE_GENERATOR}"
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_INSTALL_PREFIX=${LOTTO_CICD_INSTALL_PREFIX}
        -DLOTTO_FRONTEND=${LOTTO_FRONTEND} -DLOTTO_LTO=${LOTTO_LTO}
        -DLOTTO_RACKET_TESTS=OFF
    COMMAND ${CMAKE_COMMAND} --build "${LOTTO_CICD_INSTALL_BUILD_DIR}" -j
    COMMAND ${CMAKE_COMMAND} --install "${LOTTO_CICD_INSTALL_BUILD_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${LOTTO_CICD_INSTALL_BUILD_DIR}/modules"
    COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${LOTTO_CICD_INSTALL_BUILD_DIR}/qlotto/modules"
    COMMAND "${LOTTO_CICD_INSTALL_PREFIX}/bin/lotto" modules
    USES_TERMINAL)

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

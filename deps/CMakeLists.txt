# Include pkgtools before libvsync, to make sure we determine which pkgtools
# version is used.
include(ExternalProject)

# ------------------------------------------------------------------------------
# Dice
# ------------------------------------------------------------------------------
add_subdirectory(dice)

# ------------------------------------------------------------------------------
# tsano
# ------------------------------------------------------------------------------
install(TARGETS tsano DESTINATION lib)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/blob-tsano.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include
    COMMAND cd dice/deps/tsano && xxd -i libtsano.so
            ${CMAKE_BINARY_DIR}/include/blob-tsano.h
    DEPENDS tsano)

add_custom_target(blob-tsano DEPENDS ${CMAKE_BINARY_DIR}/include/blob-tsano.h)

# ------------------------------------------------------------------------------
# capstone
# ------------------------------------------------------------------------------
if("${LOTTO_FRONTEND}" STREQUAL "UVMM" OR "${LOTTO_FRONTEND}" STREQUAL "QEMU")
    set(CAPSTONE_ARCHITECTURE_DEFAULT OFF)
    set(CAPSTONE_ARM64_SUPPORT ON)
    set(CAPSTONE_DIET ON)

    add_subdirectory(capstone)
    target_compile_options(capstone PRIVATE -Wno-switch-default)
    target_compile_options(capstone PRIVATE -Wno-pointer-arith)
    target_compile_options(capstone PRIVATE -Wno-implicit-fallthrough)
    target_compile_options(capstone PRIVATE -Wno-unused-parameter)
    target_compile_options(capstone PRIVATE -Wno-ignored-qualifiers)
    target_compile_options(capstone PRIVATE -Wno-sign-compare)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(capstone PRIVATE -Wno-suggest-attribute=format)
    endif()
endif()

# ------------------------------------------------------------------------------
# tikl
# ------------------------------------------------------------------------------
find_program(MAKE make gmake)
if(NOT MAKE)
    message(FATAL "Cannot find make")
endif()

set(TIKL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tikl)
set(PREFIX ${CMAKE_BINARY_DIR})
ExternalProject_Add(
    tikl-build
    SOURCE_DIR ${TIKL_SRC}
    BINARY_DIR ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${MAKE} -C ${TIKL_SRC} CC=cc all
    INSTALL_COMMAND ${MAKE} -C ${TIKL_SRC} PREFIX=${PREFIX}
                    install
    BUILD_BYPRODUCTS ${PREFIX}/bin/tikl
    DEPENDS "")

set(TIKL_PROGRAM
    ${PREFIX}/bin/tikl
    PARENT_SCOPE)

set(TIKL_CHECK_PROGRAM
    ${PREFIX}/bin/tikl-check
    PARENT_SCOPE)

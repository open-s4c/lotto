# Include pkgtools before libvsync, to make sure we determine which pkgtools
# version is used.
include(ExternalProject)

#v_add_submodule(pkgtools)

add_subdirectory(dice)

#v_ensure_updated_submodule(chibi-scheme)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/chibi-scheme
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme/src)

if("${LOTTO_CHIBI}")
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CFLAGS_CHIBI "-Wno-parentheses-equality -Wno-unevaluated-expression -Wno-self-assign -Wno-implicit-const-int-float-conversion")
    endif()
    ExternalProject_Add(
        libchibi-scheme
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme/src/chibi-scheme
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            cd src/chibi-scheme && make SEXP_USE_DL=0 CC=${CMAKE_C_COMPILER} CFLAGS=${CFLAGS_CHIBI}
            AR=${CMAKE_AR} LD=${CMAKE_C_LINK_EXECUTABLE} libchibi-scheme.a
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme/src/chibi-scheme/libchibi-scheme.a
        DEPENDS "")

    set_target_properties(libchibi-scheme PROPERTIES C_CLANG_TIDY "")
    add_library(chibi-scheme STATIC IMPORTED GLOBAL)
    add_dependencies(chibi-scheme libchibi-scheme)
    set_target_properties(
        chibi-scheme
        PROPERTIES
            IMPORTED_LOCATION
            ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme/src/chibi-scheme/libchibi-scheme.a
    )
    target_include_directories(
        chibi-scheme
        INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/chibi-scheme/src/chibi-scheme/include)
endif()


# ##############################################################################
# libtsano
# ##############################################################################
install(TARGETS tsano DESTINATION lib)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/include/blob-tsano.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include
    COMMAND cd dice/deps/tsano && xxd -i libtsano.so ${CMAKE_BINARY_DIR}/include/blob-tsano.h
    DEPENDS tsano)

add_custom_target(blob-tsano DEPENDS ${CMAKE_BINARY_DIR}/include/blob-tsano.h)

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
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(capstone PRIVATE -Wno-suggest-attribute=format)
    endif()
endif()

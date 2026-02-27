sanitize()
include_directories(include)
add_subdirectory(frontend)
add_subdirectory(gdb)
add_subdirectory(measure)
add_subdirectory(snapshot)

# ##############################################################################
# QEMU config
# ##############################################################################
# set(QEMU_URL http://download.qemu.org/qemu-9.1.3.tar.xz)
set(QEMU_URL https://github.com/qemu/qemu/archive/refs/tags/v9.1.3.tar.gz)
set(QEMU_HASH 480a77a0ed13a9b39415f639aa020b4eb0d7cc5a52569510dfd830b3af1bac89)

# Build QEmu

set(QEMU_SRC ${PROJECT_SOURCE_DIR}/deps/qemu)
set(EXTRA_CFLAGS "-I${PROJECT_SOURCE_DIR}/src/include")
if((CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL
                                12.0) OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wno-error=dangling-pointer")
    set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wno-dangling-pointer")
endif()
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/include")
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wl,--export-dynamic")

ExternalProject_Add(
    qemu
    SOURCE_DIR ${QEMU_SRC}
    URL ${QEMU_URL}
    URL_HASH SHA256=${QEMU_HASH}
    DOWNLOAD_EXTRACT_TIMESTAMP YES
    PATCH_COMMAND patch -p 1 < ${PROJECT_SOURCE_DIR}/patches/qemu-9.1.3.diff
    CONFIGURE_COMMAND
        ${QEMU_SRC}/configure --target-list=aarch64-softmmu --disable-capstone
        --prefix=${PROJECT_BINARY_DIR} "--extra-cflags=${EXTRA_CFLAGS}" >
        configure.log 2>&1
    BUILD_COMMAND make -j
    BUILD_ALWAYS on
    INSTALL_COMMAND make install > install.log 2>&1)

add_library(qlotto.o OBJECT qlotto.c)
target_link_libraries(qlotto.o PRIVATE lotto.h lotto-src.h dice.h)

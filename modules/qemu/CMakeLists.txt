find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED IMPORTED_TARGET glib-2.0)

include_directories(include)

sanitize()

# QEMU source build configuration set(QEMU_URL
# http://download.qemu.org/qemu-9.1.3.tar.xz)
set(QEMU_URL https://github.com/qemu/qemu/archive/refs/tags/v9.1.3.tar.gz)
set(QEMU_HASH 79f97e8ba4a19d2b5fa9fd080379abc061466d66b12300a7fc5b5fa345ae3ac2)

set(QEMU_SRC ${PROJECT_SOURCE_DIR}/deps/qemu)
set(EXTRA_CFLAGS "")
if((CMAKE_COMPILER_IS_GNUCC AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL
                                12.0) OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wno-error=dangling-pointer")
    set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wno-dangling-pointer")
endif()
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/include")
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/qemu/include")
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/bias/include")
set(EXTRA_CFLAGS
    "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/deadlock/include")
set(EXTRA_CFLAGS
    "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/terminate/include")
set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/evec/include")
set(EXTRA_CFLAGS
    "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/mutex/include")
set(EXTRA_CFLAGS
    "${EXTRA_CFLAGS} -I${PROJECT_SOURCE_DIR}/modules/yield/include")
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

add_subdirectory(src)
add_subdirectory(test)

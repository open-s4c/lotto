include(ExternalProject)
# cmake_minimum_required(VERSION 3.20) project(lotto-qemu-tests LANGUAGES C ASM)
# ExternalProject_Add( integration-arm64-linux SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR} BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/arm64-linux
# BUILD_ALWAYS 1 CMAKE_ARGS
# -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_SOURCE_DIR}/cmake/arm64.cmake
# -DCMAKE_C_FLAGS="-static" -DLOTTO_MODULE_qemu=ON INSTALL_COMMAND "")

# Bare-metal AArch64 kernel used for qlotto integration tests. Built with the
# aarch64-baremetal toolchain (no libc, no OS). Each test produces a
# kernel-<name>.elf that can be run with: qemu-system-aarch64 -machine virt -cpu
# cortex-a57 -smp 4 -m 128M \ -nographic -no-reboot -kernel
# <build>/kernel-<name>.elf or via: lotto qemu -- -kernel
# <build>/kernel-<name>.elf
ExternalProject_Add(
    kernel-tests-aarch64
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/kernel
    BUILD_ALWAYS 1
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_SOURCE_DIR}/cmake/aarch64-baremetal.cmake
        -DQEMU_MODULE_INCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../include
        -DLOTTO_PUBLIC_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
        -DQEMU_PROFILE_MODULE_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/modules/qemu_profile/include
    INSTALL_COMMAND "")

# Top-level *.c files in this directory are bare-metal kernel test sources
# compiled by kernel-tests-aarch64, not host-side tikl launcher binaries.

set(TIKL_SRCS
    0001-kernel-1core-pass.c 0002-kernel-1core-fail.c 0003-kernel-2core-pass.c
    0005-kernel-4core-replay.c 0007-kernel-4core-terminal-race.c)

foreach(SRC ${TIKL_SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG}
                ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} kernel-tests-aarch64 tikl-build lotto-cli)
    add_dependencies(tikl-module-${MODULE_NAME} tikl-${TEST})
endforeach()

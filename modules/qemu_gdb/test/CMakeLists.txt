include(ExternalProject)

add_executable(qemu-gdb-rsp-client rsp_client.c)
target_compile_features(qemu-gdb-rsp-client PRIVATE c_std_11)
set_target_properties(
    qemu-gdb-rsp-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_CURRENT_BINARY_DIR})

ExternalProject_Add(
    kernel-tests-aarch64-qemu-gdb
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/kernel
    BUILD_ALWAYS 1
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_SOURCE_DIR}/cmake/aarch64-baremetal.cmake
        -DLOTTO_PUBLIC_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
        -DQEMU_MODULE_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/modules/qemu/include
        -DQEMU_PROFILE_MODULE_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/modules/qemu_profile/include
        -DQEMU_KERNEL_SHARED_DIR=${CMAKE_SOURCE_DIR}/modules/qemu/test/kernel
    INSTALL_COMMAND "")

set(_QEMU_GDB_KERNEL ${CMAKE_CURRENT_BINARY_DIR}/kernel/0001-kernel-gdb-abc.elf)
set(_QEMU_GDB_CLIENT ${CMAKE_CURRENT_BINARY_DIR}/qemu-gdb-rsp-client)
set(_QEMU_GDB_RUN_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/run-0001-kernel-gdb-abc.sh)

configure_file(run-0001-kernel-gdb-abc.sh.in ${_QEMU_GDB_RUN_SCRIPT} @ONLY)

add_custom_target(tikl-0001-kernel-gdb-abc COMMAND /usr/bin/bash
                                                   ${_QEMU_GDB_RUN_SCRIPT})
add_dependencies(tikl-0001-kernel-gdb-abc kernel-tests-aarch64-qemu-gdb
                 qemu-gdb-rsp-client deadline lotto)
add_dependencies(tikl-module-${MODULE_NAME} tikl-0001-kernel-gdb-abc)

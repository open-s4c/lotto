include(ExternalProject)

ExternalProject_Add(
    kernel-tests-aarch64-qemu-snapshot
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/kernel
    BUILD_ALWAYS 1
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_SOURCE_DIR}/cmake/aarch64-baremetal.cmake
        -DLOTTO_PUBLIC_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
        -DQEMU_MODULE_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/modules/qemu/include
        -DQEMU_KERNEL_SHARED_DIR=${CMAKE_SOURCE_DIR}/modules/qemu/test/kernel
    INSTALL_COMMAND "")

set(TIKL_SRCS 0001-kernel-2core-snapshot-race.c)

foreach(SRC ${TIKL_SRCS})
    get_filename_component(TEST ${SRC} NAME_WLE)
    add_custom_target(
        tikl-${TEST}
        COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG}
                ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(tikl-${TEST} kernel-tests-aarch64-qemu-snapshot tikl-build
                     lotto-cli)
    add_dependencies(tikl-module-${MODULE_NAME} tikl-${TEST})
endforeach()

cmake_minimum_required(VERSION 3.20)
project(lotto-qemu-gdb-kernel-tests LANGUAGES C ASM)

if(NOT DEFINED QEMU_KERNEL_SHARED_DIR)
    message(FATAL_ERROR "QEMU_KERNEL_SHARED_DIR must be provided by parent build")
endif()

add_compile_options(-march=armv8-a -ffreestanding -fno-builtin -Wall -O1 -g)
add_link_options(-march=armv8-a -nostdlib -nostartfiles -T
                 ${QEMU_KERNEL_SHARED_DIR}/link.ld)

if(DEFINED LOTTO_PUBLIC_INCLUDE_DIR)
    include_directories(${LOTTO_PUBLIC_INCLUDE_DIR})
endif()
if(DEFINED QEMU_MODULE_INCLUDE_DIR)
    include_directories(${QEMU_MODULE_INCLUDE_DIR})
endif()
if(DEFINED QEMU_PROFILE_MODULE_INCLUDE_DIR)
    include_directories(${QEMU_PROFILE_MODULE_INCLUDE_DIR})
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../qemu_snapshot/include)
include_directories(${QEMU_KERNEL_SHARED_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../bias/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../deadlock/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../mutex/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../terminate/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../yield/include)

set(KERNEL_SRCS ${QEMU_KERNEL_SHARED_DIR}/boot.S ${QEMU_KERNEL_SHARED_DIR}/main.c)

function(add_kernel_test TARGET CORES OUTPUT SRC)
    add_executable(${TARGET} ${KERNEL_SRCS} ${SRC})
    target_compile_definitions(${TARGET} PRIVATE NUM_CORES=${CORES})
    set_target_properties(
        ${TARGET} PROPERTIES OUTPUT_NAME "${OUTPUT}.elf"
                             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()

add_kernel_test(kernel-0001-gdb-abc 3 0001-kernel-gdb-abc ../0001-kernel-gdb-abc.c)

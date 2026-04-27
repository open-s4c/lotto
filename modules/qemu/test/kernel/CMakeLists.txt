cmake_minimum_required(VERSION 3.20)
project(lotto-kernel-tests LANGUAGES C ASM)

# Bare-metal flags: no standard library, no startup files, freestanding C.
add_compile_options(-march=armv8-a -ffreestanding -fno-builtin -Wall -O1 -g)

add_link_options(-march=armv8-a -nostdlib -nostartfiles -T
                 ${CMAKE_CURRENT_SOURCE_DIR}/link.ld)

# QEMU_MODULE_INCLUDE_DIR is set by the parent ExternalProject_Add so that tests
# can include <lotto/qemu/lotto_udf.h> for UDF annotations.
if(DEFINED QEMU_MODULE_INCLUDE_DIR)
    include_directories(${QEMU_MODULE_INCLUDE_DIR})
endif()
if(DEFINED LOTTO_PUBLIC_INCLUDE_DIR)
    include_directories(${LOTTO_PUBLIC_INCLUDE_DIR})
endif()
if(DEFINED QEMU_PROFILE_MODULE_INCLUDE_DIR)
    include_directories(${QEMU_PROFILE_MODULE_INCLUDE_DIR})
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../qemu_snapshot/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../bias/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../deadlock/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../mutex/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../terminate/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../yield/include)

# Kernel objects shared by every test binary.
set(KERNEL_SRCS boot.S main.c)

#
# add_kernel_test(TARGET CORES OUTPUT SRC)
#
# Produces an ELF named OUTPUT.elf from the shared kernel objects plus SRC.
#
function(add_kernel_test TARGET CORES OUTPUT SRC)
    add_executable(${TARGET} ${KERNEL_SRCS} ${SRC})
    target_compile_definitions(${TARGET} PRIVATE NUM_CORES=${CORES})
    set_target_properties(
        ${TARGET} PROPERTIES OUTPUT_NAME "${OUTPUT}.elf"
                             RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endfunction()

# ---- Tests -----------------------------------------------------------

add_kernel_test(kernel-0000-no-lotto-1core 1 0000-kernel-1core-no-lotto
                ../0000-kernel-1core-no-lotto.c)
add_kernel_test(kernel-0000-no-lotto-2core 2 0000-kernel-2core-no-lotto
                ../0000-kernel-2core-no-lotto.c)
add_kernel_test(kernel-0000-no-lotto-4core 4 0000-kernel-4core-no-lotto
                ../0000-kernel-4core-no-lotto.c)

add_kernel_test(kernel-0001-pass-1core 1 0001-kernel-1core-pass
                ../0001-kernel-1core-pass.c)
add_kernel_test(kernel-0002-fail-1core 1 0002-kernel-1core-fail
                ../0002-kernel-1core-fail.c)
add_kernel_test(kernel-0003-pass-2core 2 0003-kernel-2core-pass
                ../0003-kernel-2core-pass.c)
add_kernel_test(kernel-0004-udf-2core 2 0004-kernel-2core-udf
                ../0004-kernel-2core-udf.c)
add_kernel_test(kernel-0005-replay-4core 4 0005-kernel-4core-replay
                ../0005-kernel-4core-replay.c)
add_kernel_test(kernel-0006-mp 4 0006-kernel-mp ../0006-kernel-mp.c)
add_kernel_test(
    kernel-0007-terminal-race-4core 4 0007-kernel-4core-terminal-race
    ../0007-kernel-4core-terminal-race.c)

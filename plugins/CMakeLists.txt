link_libraries(dice.h)
if(${LOTTO_LTO})
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    add_compile_options(-fno-strict-aliasing -Wall)
endif()

# already executed by /src/CMakeLists.txt execute_process(COMMAND bash
# ${CMAKE_SOURCE_DIR}/scripts/gen_ps_topics.sh ${CMAKE_BINARY_DIR}
# OUTPUT_VARIABLE PS_INTERFACE_TOPICS)

gen_pubsub_headers("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
include_directories(${CMAKE_BINARY_DIR}/generated)
add_compile_definitions(_GNU_SOURCE)

add_subdirectory(leakcheck)
add_subdirectory(fork_execve)
add_subdirectory(mempool)
add_subdirectory(priority)
add_subdirectory(qemu)
add_subdirectory(rust)
add_subdirectory(uaf)

find_program(CARGO_EXECUTABLE cargo)
find_program(RUSTUP_EXECUTABLE rustup)

set(LOTTO_RUST_NIGHTLY_TIKL_FEATURE_ARG "")
if(CARGO_EXECUTABLE AND RUSTUP_EXECUTABLE)
    execute_process(
        COMMAND ${RUSTUP_EXECUTABLE} run nightly cargo --version
        RESULT_VARIABLE LOTTO_CARGO_NIGHTLY_RESULT
        OUTPUT_QUIET ERROR_QUIET)
    if(LOTTO_CARGO_NIGHTLY_RESULT EQUAL 0)
        set(LOTTO_RUST_NIGHTLY_TIKL_FEATURE_ARG "-D rust-nightly")
    endif()
endif()

if(APPLE)
    set(TIKL_STDBUF)
else()
    set(TIKL_STDBUF "stdbuf -i0 -o0 -e0 ")
endif()

set(LOTTO_RUST_TIKL_TIMEOUT
    600
    CACHE STRING "tikl timeout for Rust tests")

set(TIKL_CFG ${CMAKE_CURRENT_BINARY_DIR}/tikl.cfg)
configure_file(tikl.cfg.in ${TIKL_CFG} @ONLY)

set(RUST_TIKL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tokio_replay.sh)

add_custom_target(
    tikl-rust
    COMMAND ${TIKL_PROGRAM} -c ${TIKL_CFG} ${RUST_TIKL_SRCS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(tikl-rust tikl-build lotto-cli)
add_dependencies(tikl-test tikl-rust)

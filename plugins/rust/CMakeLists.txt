if(NOT "${LOTTO_RUST}")
    return()
endif()

# ##############################################################################
# Import Corrosion
# ##############################################################################

include(FetchContent)

FetchContent_Declare(
    Corrosion GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git)
execute_process(COMMAND rustup which rustc OUTPUT_VARIABLE Rust_COMPILER)
string(STRIP ${Rust_COMPILER} Rust_COMPILER)
FetchContent_MakeAvailable(Corrosion)

# ##############################################################################
# Import Crates
# ##############################################################################
set(CARGO_PROFILE "RelWithDbg")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CARGO_PROFILE "dev")
endif()

if(NOT DEFINED LOTTO_RUST_DEMO)
    set(LOTTO_RUST_DEMO OFF)
endif()
if("${LOTTO_RUST_DEMO}")
    list(APPEND FEATURES "demo")
endif()
if("${LOTTO_STABLE_ADDRESS_MAP}")
    list(APPEND FEATURES "stable_address_map")
endif()

if("${LOTTO_FRONTEND}" STREQUAL "QEMU")
    list(APPEND FEATURES "qlotto")
endif()

# cmake-format: off
corrosion_import_crate(
    MANIFEST_PATH "Cargo.toml"
    PROFILE       "${CARGO_PROFILE}"
    CRATES        rusty_engine
                  rusty_cli
    FEATURES      ${FEATURES})
# cmake-format: on

# ##############################################################################
# Plugins
# ##############################################################################
add_subdirectory(runtime)
add_subdirectory(engine)
add_subdirectory(cli)

# ##############################################################################
# Cargo commands
# ##############################################################################
add_custom_target(
    rust-format-check
    COMMAND cargo fmt --all --check
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
    rust-format-apply
    COMMAND cargo fmt --all
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
    rust-doc
    COMMAND cargo doc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
    rust-lint-check
    COMMAND cargo clippy --all
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
    rust-lint-apply
    COMMAND cargo clippy --all --fix --allow-dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
    rust-test
    COMMAND cargo test --workspace --exclude rusty_engine --exclude rusty_cli
            --all-features
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# ##############################################################################
# Support Cargo test
# ##############################################################################
add_library(lotto_rust_testing STATIC)
target_link_libraries(
    lotto_rust_testing
    PRIVATE sys_testing.o
            base_testing.o
            engine_testing.o
            cli_testing.o
            brokers_testing.o
            states.o
            flags_prng_testing.o
            flags_sequencer_testing.o
            memmgr_runtime.o
            memmgr_user.o)
add_dependencies(rust-test lotto_rust_testing)
